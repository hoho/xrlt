#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <xrlt.h>

#define DDEBUG 1
#include "ddebug.h"
#include "ngx_buf.h"


typedef struct {
    u_char                    *name;
    ngx_http_complex_value_t   value;
} ngx_http_xrlt_param_t;


typedef struct {
    xrltRequestsheetPtr        sheet;
    //ngx_hash_t                 types;
    //ngx_array_t               *types_keys;
    ngx_array_t               *params;       /* ngx_http_xrlt_param_t */
} ngx_http_xrlt_loc_conf_t;


static ngx_int_t   ngx_http_xrlt_init          (ngx_conf_t *cf);
static ngx_int_t   ngx_http_xrlt_filter_init   (ngx_conf_t *cf);
static char       *ngx_http_xrlt               (ngx_conf_t *cf,
                                                ngx_command_t *cmd, void *conf);
static char       *ngx_http_xrlt_param         (ngx_conf_t *cf,
                                                ngx_command_t *cmd, void *conf);
static void       *ngx_http_xrlt_create_conf   (ngx_conf_t *cf);
static char       *ngx_http_xrlt_merge_conf    (ngx_conf_t *cf, void *parent,
                                                void *child);
static void        ngx_http_xrlt_exit          (ngx_cycle_t *cycle);


ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_command_t ngx_http_xrlt_commands[] = {
    { ngx_string("xrlt_param"),
      NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF
                         | NGX_HTTP_LIF_CONF |NGX_CONF_TAKE2,
      ngx_http_xrlt_param,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("xrlt"),
      NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
      ngx_http_xrlt,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
    ngx_null_command
};


ngx_http_module_t ngx_http_xrlt_module_ctx = {
        ngx_http_xrlt_init,               /*  preconfiguration */
        ngx_http_xrlt_filter_init,        /*  postconfiguration */

        NULL,                             /*  create main configuration */
        NULL,                             /*  init main configuration */

        NULL,                             /*  create server configuration */
        NULL,                             /*  merge server configuration */

        ngx_http_xrlt_create_conf,        /*  create location configuration */
        ngx_http_xrlt_merge_conf          /*  merge location configuration */
};


ngx_module_t ngx_http_xrlt_module = {
        NGX_MODULE_V1,
        &ngx_http_xrlt_module_ctx,  /*  module context */
        ngx_http_xrlt_commands,     /*  module directives */
        NGX_HTTP_MODULE,            /*  module type */
        NULL,                       /*  init master */
        NULL,                       /*  init module */
        NULL,                       /*  init process */
        NULL,                       /*  init thread */
        NULL,                       /*  exit thread */
        ngx_http_xrlt_exit,         /*  exit process */
        ngx_http_xrlt_exit,         /*  exit master */
        NGX_MODULE_V1_PADDING
};


typedef struct ngx_http_xrlt_ctx_s {
    xrltContextPtr   xctx;
    ngx_int_t        id;
} ngx_http_xrlt_ctx_t;


static void
ngx_http_xrlt_cleanup_context(void *data)
{
    xrltContextFree(data);
}


ngx_inline static ngx_http_xrlt_ctx_t *
ngx_http_xrlt_create_ctx(ngx_http_request_t *r, ngx_int_t id) {
    ngx_http_xrlt_ctx_t       *ctx;
    ngx_http_xrlt_loc_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(r, ngx_http_xrlt_module);

    ctx = ngx_palloc(r->pool, sizeof(ngx_http_xrlt_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    if (id == 0) {
        ngx_pool_cleanup_t  *cln;

        cln = ngx_pool_cleanup_add(r->pool, 0);
        if (cln == NULL) {
            return NULL;
        }

        ctx->xctx = xrltContextCreate(conf->sheet);
        ctx->id = 0;

        if (ctx->xctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "Failed to create XRLT context");
        }

        cln->handler = ngx_http_xrlt_cleanup_context;
        cln->data = ctx->xctx;
    } else {
        ngx_http_xrlt_ctx_t  *main_ctx;

        main_ctx = ngx_http_get_module_ctx(r->main, ngx_http_xrlt_module);

        if (main_ctx == NULL || main_ctx->xctx == NULL) {
            return NULL;
        }

        ctx->xctx = main_ctx->xctx;
        ctx->id = id;
    }

    return ctx;
}


ngx_inline static ngx_int_t
ngx_http_xrlt_transform(ngx_http_request_t *r, ngx_http_xrlt_ctx_t *ctx,
                        ngx_chain_t *in)
{
    int          t;
    xrltString   s;

    if (in == NULL) {
        t = xrltTransform(ctx->xctx, 0, NULL);
    } else {
        xrltTransformValue   val;

        val.type = XRLT_PROCESS_BODY;
        val.last = in->buf->last ? TRUE : FALSE;
        val.error = FALSE;
        val.data.data = (char *)in->buf->pos;
        val.data.len = in->buf->last - in->buf->pos;

        fprintf(stderr, "FUFUFUFUFU: %s\n", val.data.data);

        t = xrltTransform(ctx->xctx, ctx->id, &val);
    }

    if (t & XRLT_STATUS_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (t == XRLT_STATUS_WAITING) {
        // Nothing to do, just wait.
        return NGX_AGAIN;
    }

    if (t & XRLT_STATUS_REFUSE_SUBREQUEST) {
        // Cancel r subrequest, there is something wrong with it.
    }

    if (t & XRLT_STATUS_SUBREQUEST) {
        size_t                   id;
        xrltHTTPMethod           m;
        xrltSubrequestDataType   type;
        xrltHeaderList           header;
        xrltString               url, querystring, body, name, val;
        ngx_http_xrlt_ctx_t     *sr_ctx;
        ngx_str_t                sr_uri;
        ngx_str_t                sr_querystring;
        ngx_http_request_t      *sr;

        while (xrltSubrequestListShift(&ctx->xctx->sr, &id, &m, &type, &header,
                                       &url, &querystring, &body))
        {
            sr_ctx = ngx_http_xrlt_create_ctx(r, id);
            if (sr_ctx == NULL) {
                xrltHeaderListClear(&header);

                xrltStringClear(&url);
                xrltStringClear(&querystring);
                xrltStringClear(&body);

                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            sr_uri.data = (u_char *)url.data;
            sr_uri.len = url.len;

            sr_querystring.data = (u_char *)querystring.data;
            sr_querystring.len = querystring.len;

            fprintf(stderr, "SR: %s %s\n", sr_uri.data, sr_querystring.data);

            if (ngx_http_subrequest(r, &sr_uri, &sr_querystring, &sr,
                                    NULL, 0) != NGX_OK)
            {
                xrltHeaderListClear(&header);

                xrltStringClear(&url);
                xrltStringClear(&querystring);
                xrltStringClear(&body);

                return NGX_ERROR;
            }

            ngx_http_set_ctx(sr, sr_ctx, ngx_http_xrlt_module);

            while (xrltHeaderListShift(&header, &name, &val)) {
                xrltStringClear(&name);
                xrltStringClear(&val);
            }

            xrltStringClear(&url);
            xrltStringClear(&querystring);
            xrltStringClear(&body);
        }

    }

    if (t & XRLT_STATUS_CHUNK) {
        ngx_buf_t    *b;
        ngx_chain_t   out;

        while (xrltChunkListShift(&ctx->xctx->chunk, &s)) {
            if (s.len > 0) {
                fprintf(stderr, "CHUNK: %s\n", s.data);
                b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

                b->start = b->pos = (u_char *)s.data;
                b->last = b->end = (u_char *)s.data + s.len;

                b->last_buf = 0;
                b->memory = 1;
                b->last_in_chain = 1;
                b->flush = 1;

                out.buf = b;
                out.next = NULL;

                if (ngx_http_output_filter(r->main, &out) == NGX_ERROR) {
                    return NGX_ERROR;
                }
            }

            xrltStringClear(&s);
        }

    }

    if (t & XRLT_STATUS_LOG) {
        xrltLogType   ltype;

        while (xrltLogListShift(&ctx->xctx->log, &ltype, &s)) {
            xrltStringClear(&s);
        }
    }

    if (t & XRLT_STATUS_DONE) {
        if (ngx_http_send_special(r->main, NGX_HTTP_LAST) == NGX_ERROR) {
            return NGX_ERROR;
        }

        return NGX_DONE;
    }

    return ngx_http_xrlt_transform(r, ctx, NULL);
}


static ngx_int_t
ngx_http_xrlt_header_filter(ngx_http_request_t *r)
{
    ngx_http_xrlt_ctx_t         *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (r == r->main || ctx == NULL) {
        return ngx_http_next_header_filter(r);
    }

    dd("HEFIFI");

    return NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_xrlt_ctx_t  *ctx;
    ngx_chain_t          *cl;
    ngx_int_t             rc;

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (in == NULL || r == r->main || ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    rc = ngx_http_xrlt_transform(r, ctx, in);
    if (rc == NGX_HTTP_INTERNAL_SERVER_ERROR || rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    for (cl = in; cl; cl = cl->next) {
        cl->buf->pos = cl->buf->last;
        cl->buf->file_pos = cl->buf->file_last;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_xrlt_init(ngx_conf_t *cf)
{
    xmlInitParser();
    xrltInit();

    return NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_xrlt_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_xrlt_body_filter;

    return NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_handler(ngx_http_request_t *r) {
    dd("FUCKFUCK");

    ngx_http_xrlt_ctx_t  *ctx;
    ngx_int_t             rc;

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (ctx == NULL) {
        ctx = ngx_http_xrlt_create_ctx(r, 0);
        if (ctx == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_xrlt_module);
    }

    r->headers_out.status = NGX_HTTP_OK;

    ngx_http_send_header(r);

    rc = ngx_http_xrlt_transform(r, ctx, NULL);
    if (rc == NGX_HTTP_INTERNAL_SERVER_ERROR || rc == NGX_ERROR) {
        return rc;
    }

    return NGX_OK;
}


static void
ngx_http_xrlt_cleanup_requestsheet(void *data)
{
    xrltRequestsheetFree(data);
}


static char *
ngx_http_xrlt(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_xrlt_loc_conf_t  *xlcf = conf;
    ngx_http_core_loc_conf_t  *clcf;

    ngx_str_t                 *value;
    ngx_pool_cleanup_t        *cln;

    xmlDocPtr                 doc;
    xrltRequestsheetPtr       sheet;

    value = cf->args->elts;

    if (xlcf->sheet != NULL) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "Duplicate xrlt instruction");
        return NGX_CONF_ERROR;
    }

    if (ngx_conf_full_name(cf->cycle, &value[1], 0) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NGX_CONF_ERROR;
    }


    doc = xmlParseFile((const char *)value[1].data);
    if (doc == NULL) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "xmlParseFile(\"%s\") failed", value[1].data);
        return NGX_CONF_ERROR;
    }

    sheet = xrltRequestsheetCreate(doc);
    if (sheet == NULL) {
        xmlFreeDoc(doc);
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "xrltRequestsheetCreate(\"%s\") failed",
                           value[1].data);
        return NGX_CONF_ERROR;
    }

    xlcf->sheet = sheet;

    cln->handler = ngx_http_xrlt_cleanup_requestsheet;
    cln->data = sheet;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (clcf == NULL) {
        return NGX_CONF_ERROR;
    }

    clcf->handler = ngx_http_xrlt_handler;

    return NGX_CONF_OK;
}


static char *
ngx_http_xrlt_param(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_xrlt_loc_conf_t          *xlcf = conf;

    ngx_http_xrlt_param_t             *param;
    ngx_http_compile_complex_value_t   ccv;
    ngx_str_t                         *value;

    value = cf->args->elts;

    if (xlcf->params == NULL) {
        xlcf->params = ngx_array_create(cf->pool, 2,
                                        sizeof(ngx_http_xrlt_param_t));
        if (xlcf->params == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    param = ngx_array_push(xlcf->params);
    if (param == NULL) {
        return NGX_CONF_ERROR;
    }

    param->name = value[1].data;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &param->value;
    ccv.zero = 1;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static void *
ngx_http_xrlt_create_conf(ngx_conf_t *cf)
{
    ngx_http_xrlt_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_xrlt_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}


static char *
ngx_http_xrlt_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_xrlt_loc_conf_t  *prev = parent;
    ngx_http_xrlt_loc_conf_t  *conf = child;

    if (conf->params == NULL) {
        conf->params = prev->params;
    }

    return NGX_CONF_OK;
}


static void
ngx_http_xrlt_exit(ngx_cycle_t *cycle)
{
    //xsltCleanupGlobals();
    xmlCleanupParser();
}
