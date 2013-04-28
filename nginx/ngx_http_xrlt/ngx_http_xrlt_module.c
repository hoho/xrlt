#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <xrlt.h>

#define DDEBUG 1
#include "ddebug.h"


ngx_str_t   ngx_http_xrlt_head_method =    { 4, (u_char *) "HEAD " };
ngx_str_t   ngx_http_xrlt_post_method =    { 4, (u_char *) "POST " };
ngx_str_t   ngx_http_xrlt_put_method =     { 3, (u_char *) "PUT " };
ngx_str_t   ngx_http_xrlt_delete_method =  { 6, (u_char *) "DELETE " };
ngx_str_t   ngx_http_xrlt_trace_method =   { 5, (u_char *) "TRACE " };
ngx_str_t   ngx_http_xrlt_options_method = { 7, (u_char *) "OPTIONS " };


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
static ngx_int_t   ngx_http_xrlt_post_sr       (ngx_http_request_t *r,
                                                void *data, ngx_int_t rc);


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


#define XRLT_STR_2_NGX_STR(dst, src) {                                        \
    dst.len = src.len;                                                        \
    if (src.len > 0) {                                                        \
        dst.data = ngx_pnalloc(r->pool, src.len);                             \
        if (dst.data == NULL) return NGX_ERROR;                               \
        (void)ngx_copy(dst.data, src.data, src.len);                          \
    } else {                                                                  \
        dst.data = NULL;                                                      \
    }                                                                         \
}


ngx_inline static ngx_int_t
ngx_http_xrlt_transform(ngx_http_request_t *r, ngx_http_xrlt_ctx_t *ctx,
                        ngx_chain_t *in, ngx_int_t last)
{
    int          t;
    xrltString   s;

    if (in == NULL && !last) {
        t = xrltTransform(ctx->xctx, 0, NULL);
    } else {
        xrltTransformValue   val;

        val.type = XRLT_PROCESS_BODY;
        val.last = last ? TRUE : FALSE;
        val.error = FALSE;

        if (in == NULL) {
            val.data.data = NULL;
            val.data.len = 0;
        } else {
            val.data.data = (char *)in->buf->pos;
            val.data.len = in->buf->last - in->buf->pos;
        }

        t = xrltTransform(ctx->xctx, ctx->id, &val);
    }

    if (t & XRLT_STATUS_ERROR) {
        return NGX_ERROR;
    }

    if (t == XRLT_STATUS_WAITING) {
        // Nothing to do, just wait.
        return NGX_AGAIN;
    }

    if (t & XRLT_STATUS_REFUSE_SUBREQUEST) {
        // Cancel r subrequest, there is something wrong with it.
    }

    if (t & XRLT_STATUS_SUBREQUEST) {
        size_t                       id;
        xrltHTTPMethod               m;
        xrltSubrequestDataType       type;
        xrltHeaderList               header;
        xrltString                   url, querystring, body, name, val;
        ngx_http_xrlt_ctx_t         *sr_ctx;
        ngx_str_t                    sr_uri, sr_querystring, sr_body;
        ngx_http_request_t          *sr;
        ngx_http_post_subrequest_t  *psr;
        ngx_http_request_body_t     *rb;
        ngx_buf_t                   *b;

        while (xrltSubrequestListShift(&ctx->xctx->sr, &id, &m, &type, &header,
                                       &url, &querystring, &body))
        {
            dd("Srrrrrr %s", url.data);
            sr_ctx = ngx_http_xrlt_create_ctx(r, id);
            if (sr_ctx == NULL) {
                xrltHeaderListClear(&header);

                xrltStringClear(&url);
                xrltStringClear(&querystring);
                xrltStringClear(&body);

                return NGX_ERROR;
            }

            XRLT_STR_2_NGX_STR(sr_uri, url);
            XRLT_STR_2_NGX_STR(sr_querystring, querystring);
            XRLT_STR_2_NGX_STR(sr_body, body);
            xrltStringClear(&url);
            xrltStringClear(&querystring);
            xrltStringClear(&body);

            psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
            if (psr == NULL) {
                return NGX_ERROR;
            }

            psr->handler = ngx_http_xrlt_post_sr;
            psr->data = sr_ctx;

            if (ngx_http_subrequest(r->main, &sr_uri, &sr_querystring, &sr,
                                    psr, 0) != NGX_OK)
            {
                xrltHeaderListClear(&header);
                return NGX_ERROR;
            }

            switch (m) {
                case XRLT_METHOD_GET:
                    sr->method = NGX_HTTP_GET;
                    break;
                case XRLT_METHOD_HEAD:
                    sr->method = NGX_HTTP_HEAD;
                    sr->method_name = ngx_http_xrlt_head_method;
                    break;
                case XRLT_METHOD_POST:
                    sr->method = NGX_HTTP_POST;
                    sr->method_name = ngx_http_xrlt_post_method;
                    break;
                case XRLT_METHOD_PUT:
                    sr->method = NGX_HTTP_PUT;
                    sr->method_name = ngx_http_xrlt_put_method;
                    break;
                case XRLT_METHOD_DELETE:
                    sr->method = NGX_HTTP_DELETE;
                    sr->method_name = ngx_http_xrlt_delete_method;
                    break;
                case XRLT_METHOD_TRACE:
                    sr->method = NGX_HTTP_TRACE;
                    sr->method_name = ngx_http_xrlt_trace_method;
                    break;
                case XRLT_METHOD_OPTIONS:
                    sr->method = NGX_HTTP_OPTIONS;
                    sr->method_name = ngx_http_xrlt_options_method;
                    break;
                default:
                    sr->method = NGX_HTTP_GET;
            }

            while (xrltHeaderListShift(&header, &name, &val)) {
                xrltStringClear(&name);
                xrltStringClear(&val);
            }

            ngx_http_set_ctx(sr, sr_ctx, ngx_http_xrlt_module);

            if (sr_body.len > 0) {
                rb = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
                if (rb == NULL) {
                    return NGX_ERROR;
                }

                b = ngx_calloc_buf(r->pool);
                if (b == NULL) {
                    return NGX_ERROR;
                }

                rb->bufs = ngx_alloc_chain_link(r->pool);
                if (rb->bufs == NULL) {
                    return NGX_ERROR;
                }

                b->temporary = 1;
                b->start = b->pos = sr_body.data;
                b->end = b->last = sr_body.data + sr_body.len;
                b->last_buf = 1;
                b->last_in_chain = 1;

                rb->bufs->buf = b;
                rb->bufs->next = NULL;
                rb->buf = b;

                sr->request_body = rb;
            }
        }

    }

    if (t & XRLT_STATUS_CHUNK) {
        ngx_buf_t    *b;
        ngx_chain_t   out;

        while (xrltChunkListShift(&ctx->xctx->chunk, &s)) {
            if (s.len > 0) {
                b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
                if (b == NULL) {
                    xrltStringClear(&s);
                    return NGX_ERROR;
                }

                b->start = ngx_pcalloc(r->pool, s.len);
                if (b->start == NULL) {
                    xrltStringClear(&s);
                    return NGX_ERROR;
                }
                (void)ngx_copy(b->start, s.data, s.len);

                b->pos = b->start;

                b->last = b->end = b->start + s.len;
                b->temporary = 1;
                b->last_in_chain = 1;
                b->flush = 1;
                b->last_buf = 0;

                out.buf = b;
                out.next = NULL;

                if (ngx_http_output_filter(r->main, &out) == NGX_ERROR) {
                    xrltStringClear(&s);
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

        ngx_http_finalize_request(r->main, NGX_OK);

        return NGX_DONE;
    }

    return ngx_http_xrlt_transform(r, ctx, NULL, 0);
}


static ngx_int_t
ngx_http_xrlt_post_sr(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_xrlt_ctx_t         *sr_ctx = data;
    //ngx_http_request_t          *pr;
    //ngx_http_xrlt_ctx_t         *pr_ctx;

    /*pr = r->parent;

    pr_ctx = ngx_http_get_module_ctx(pr, ngx_http_xrlt_module);
    if (pr_ctx == NULL) {
        return NGX_ERROR;
    }*/

    /* work-around issues in nginx's event module */
    /*if (r != r->connection->data && r->postponed &&
            (r->main->posted_requests == NULL ||
            r->main->posted_requests->request != pr))
    {
        ngx_http_post_request(pr, NULL);
    }*/

    if (ngx_http_xrlt_transform(r, sr_ctx, NULL, 1) == NGX_ERROR) {
        return NGX_ERROR;
    }

    return rc;
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
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    if (r == r->main) {
        return ngx_http_next_body_filter(r, in);
    }

    for (cl = in; cl; cl = cl->next) {
        rc = ngx_http_xrlt_transform(r, ctx, cl, 0);
        if (rc == NGX_ERROR) {
            return NGX_ERROR;
        }

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

    r->main->count++;

    rc = ngx_http_xrlt_transform(r, ctx, NULL, 0);
    if (rc == NGX_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return rc == NGX_DONE ? NGX_OK : NGX_DONE;
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
