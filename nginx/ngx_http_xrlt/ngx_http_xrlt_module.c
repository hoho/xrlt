#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <xrlt.h>

#define DDEBUG 1
#include "ddebug.h"


static ngx_str_t   ngx_http_xrlt_head_method =    { 4, (u_char *) "HEAD " };
static ngx_str_t   ngx_http_xrlt_post_method =    { 4, (u_char *) "POST " };
static ngx_str_t   ngx_http_xrlt_put_method =     { 3, (u_char *) "PUT " };
static ngx_str_t   ngx_http_xrlt_delete_method =  { 6, (u_char *) "DELETE " };
static ngx_str_t   ngx_http_xrlt_trace_method =   { 5, (u_char *) "TRACE " };
static ngx_str_t   ngx_http_xrlt_options_method = { 7, (u_char *) "OPTIONS " };


typedef struct {
    ngx_str_t                  name;
    ngx_http_complex_value_t   value;
} ngx_http_xrlt_param_t;



typedef struct {
    xrltRequestsheetPtr        sheet;
    //ngx_hash_t                 types;
    //ngx_array_t               *types_keys;
    ngx_array_t               *params;       /* ngx_http_xrlt_param_t */
} ngx_http_xrlt_loc_conf_t;


ngx_str_t   ngx_http_xrlt_content_length_key = ngx_string("Content-Length");


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


typedef struct ngx_http_xrlt_ctx_s ngx_http_xrlt_ctx_t;
struct ngx_http_xrlt_ctx_s {
    xrltContextPtr        xctx;
    size_t                id;
    ngx_http_xrlt_ctx_t  *main_ctx;
    unsigned              headers_sent:1;
    unsigned              run_post_subrequest:1;
    unsigned              refused:1;
};


static void
ngx_http_xrlt_cleanup_context(void *data)
{
    dd("XRLT context cleanup");

    xrltContextFree(data);
}


ngx_inline static ngx_http_xrlt_ctx_t *
ngx_http_xrlt_create_ctx(ngx_http_request_t *r, size_t id) {
    ngx_http_xrlt_ctx_t       *ctx;
    ngx_http_xrlt_loc_conf_t  *conf;
    ngx_uint_t                 i, j;
    ngx_http_xrlt_param_t     *params;
    xmlChar                  **xrltparams;

    conf = ngx_http_get_module_loc_conf(r, ngx_http_xrlt_module);

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_xrlt_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    if (id == 0) {
        ngx_pool_cleanup_t  *cln;

        cln = ngx_pool_cleanup_add(r->pool, 0);
        if (cln == NULL) {
            return NULL;
        }

        dd("XRLT context creation");

        if (conf->params != NULL && conf->params->nelts > 0) {
            params = conf->params->elts;

            xrltparams = ngx_palloc(
                r->pool, sizeof(xmlChar *) * (conf->params->nelts * 2 + 1)
            );

            j = 0;

            for (i = 0; i < conf->params->nelts; i++) {
                xrltparams[j++] = xmlStrndup(params[i].name.data,
                                             params[i].name.len);
                xrltparams[j++] = xmlStrndup(params[i].value.value.data,
                                             params[i].value.value.len);
            }

            xrltparams[j] = NULL;
        } else {
            xrltparams = NULL;
        }

        ctx->xctx = xrltContextCreate(conf->sheet, xrltparams);
        ctx->id = 0;
        ctx->main_ctx = ctx;

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
        ctx->main_ctx = main_ctx;
    }

    return ctx;
}


static ngx_int_t
ngx_http_xrlt_init_subrequest_headers(ngx_http_request_t *sr, off_t len)
{
    ngx_table_elt_t  *h;
    u_char           *p;

    memset(&sr->headers_in, 0, sizeof(ngx_http_headers_in_t));

    sr->headers_in.content_length_n = len;

    if (ngx_list_init(&sr->headers_in.headers, sr->pool, 20,
                      sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    h = ngx_list_push(&sr->headers_in.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->key = ngx_http_xrlt_content_length_key;
    h->lowcase_key = ngx_pnalloc(sr->pool, h->key.len);
    if (h->lowcase_key == NULL) {
        return NGX_ERROR;
    }

    ngx_strlow(h->lowcase_key, h->key.data, h->key.len);

    sr->headers_in.content_length = h;

    p = ngx_palloc(sr->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    h->value.data = p;

    h->value.len = ngx_sprintf(h->value.data, "%O", len) - h->value.data;

    h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(
        ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(
        ngx_hash('c', 'o'), 'n'), 't'), 'e'), 'n'), 't'), '-'), 'l'), 'e'),
        'n'), 'g'), 't'), 'h');

    return NGX_OK;
}


#define XRLT_STR_2_NGX_STR(dst, src) {                                        \
    dst.len = src.len;                                                        \
    if (src.len > 0) {                                                        \
        dst.data = ngx_pnalloc(r->pool, src.len + 1);                         \
        if (dst.data == NULL) return NGX_ERROR;                               \
        (void)ngx_copy(dst.data, src.data, src.len);                          \
        dst.data[dst.len] = '\0';                                             \
    } else {                                                                  \
        dst.data = NULL;                                                      \
        dst.len = 0;                                                          \
    }                                                                         \
}


static void
ngx_http_xrlt_wev_handler(ngx_http_request_t *r)
{
    dd("XRLT end (main: %p)", r);

    if (r == r->connection->data && r->postponed) {
        ngx_http_output_filter(r, NULL);
    }

    ngx_http_finalize_request(r, NGX_OK);
}


ngx_inline static ngx_int_t
ngx_http_xrlt_process_transform_result(ngx_http_request_t *r,
                                       ngx_http_xrlt_ctx_t *ctx,
                                       int result)
{
    xrltString   s;

    if (result & XRLT_STATUS_ERROR) {
        return NGX_ERROR;
    }

    if (result == XRLT_STATUS_WAITING) {
        // Nothing to do, just wait.
        return NGX_AGAIN;
    }

    if (result & XRLT_STATUS_HEADER) {
        ngx_table_elt_t  *h;
        xrltHeaderOutType htype;
        xrltString        hname;
        xrltString        hval;
        ngx_int_t         hst;

        while (xrltHeaderOutListShift(&ctx->xctx->header, &htype, &hname,
                                      &hval))
        {
            if (ctx->headers_sent) {
                dd("Response headers are already sent");
            } else {
                if (htype == XRLT_HEADER_OUT_STATUS) {
                    hst = ngx_atoi((u_char *)hval.data, hval.len);

                    if (hst < 0) {
                        xrltStringClear(&hname);
                        xrltStringClear(&hval);
                        return NGX_ERROR;
                    }

                    r->main->headers_out.status = (ngx_uint_t)hst;
                } else {
                    h = ngx_list_push(&r->main->headers_out.headers);
                    if (h == NULL) {
                        xrltStringClear(&hname);
                        xrltStringClear(&hval);
                        return NGX_ERROR;
                    }

                    h->hash = 1;
                    XRLT_STR_2_NGX_STR(h->key, hname);
                    XRLT_STR_2_NGX_STR(h->value, hval);

                    dd("Pushing response header (name: %s, value: %s)",
                       h->key.data, h->value.data);
                }
            }

            xrltStringClear(&hname);
            xrltStringClear(&hval);
        }
    }

    if (result & XRLT_STATUS_REFUSE_SUBREQUEST) {
        // Cancel r subrequest, there is something wrong with it.
        ctx->refused = TRUE;
    }

    if (result & XRLT_STATUS_SUBREQUEST) {
        size_t                       sr_id;
        xrltHTTPMethod               m;
        xrltSubrequestDataType       type;
        xrltHeaderOutList            sr_header;
        xrltString                   url, querystring, body, hname, hval;
        xrltHeaderOutType            htype;
        ngx_http_xrlt_ctx_t         *sr_ctx;
        ngx_str_t                    sr_uri, sr_querystring, sr_body;
        ngx_http_request_t          *sr;
        ngx_http_post_subrequest_t  *psr;
        ngx_http_request_body_t     *rb;
        ngx_buf_t                   *b;
        ngx_table_elt_t             *h;
        ngx_http_core_main_conf_t   *cmcf;

        while (xrltSubrequestListShift(&ctx->xctx->sr, &sr_id, &m, &type,
                                       &sr_header, &url, &querystring, &body))
        {
            sr_ctx = ngx_http_xrlt_create_ctx(r, sr_id);
            if (sr_ctx == NULL) {
                xrltHeaderOutListClear(&sr_header);

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
                xrltHeaderOutListClear(&sr_header);
                return NGX_ERROR;
            }

            dd("Performing subrequest (r: %p, uri: %s, querystring: %s, "
               "body: %s)", sr, sr_uri.data, sr_querystring.data, sr_body.data);

            // Don't inherit parent request variables. Content-Length is
            // cacheable in proxy module and we don't need Content-Length from
            // another subrequest.
            cmcf = ngx_http_get_module_main_conf(sr, ngx_http_core_module);
            sr->variables = ngx_pcalloc(sr->pool, cmcf->variables.nelts
                                        * sizeof(ngx_http_variable_value_t));
            if (sr->variables == NULL) {
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

            if (ngx_http_xrlt_init_subrequest_headers(sr,
                                                      sr_body.len) == NGX_ERROR)
            {
                xrltHeaderOutListClear(&sr_header);
                return NGX_ERROR;
            }

            if (sr_header.first != NULL) {
                while (xrltHeaderOutListShift(&sr_header, &htype, &hname, &hval))
                {
                    h = ngx_list_push(&sr->headers_in.headers);
                    if (h == NULL) {
                        xrltStringClear(&hname);
                        xrltStringClear(&hval);
                        xrltHeaderOutListClear(&sr_header);
                        return NGX_ERROR;
                    }

                    XRLT_STR_2_NGX_STR(h->key, hname);
                    XRLT_STR_2_NGX_STR(h->value, hval);
                    xrltStringClear(&hname);
                    xrltStringClear(&hval);

                    h->lowcase_key = ngx_pnalloc(r->pool, h->key.len);
                    if (h->lowcase_key == NULL) {
                        xrltHeaderOutListClear(&sr_header);
                        return NGX_ERROR;
                    }

                    ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
                }
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

    if (result & XRLT_STATUS_CHUNK) {
        // Send response chunks out in case we don't have response headers in
        // progress or if response headers are sent.
        if (ctx->headers_sent || ctx->xctx->headerCount == 0) {
            ngx_buf_t           *b;
            ngx_chain_t          out;
            ngx_http_request_t  *ar; /* active request */
            ngx_int_t            rc;

            while (xrltChunkListShift(&ctx->xctx->chunk, &s)) {
                if (s.len > 0) {
                    if (!ctx->main_ctx->headers_sent) {
                        ctx->main_ctx->headers_sent = 1;

                        dd("Sending response headers");

                        if (ngx_http_send_header(r->main) != NGX_OK) {
                            xrltStringClear(&s);
                            return NGX_ERROR;
                        }
                    }

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

                    dd("Sending response chunk (len: %zd)", s.len);

                    ar = r->connection->data;

                    if (ar != r->main) {
                        /* bypass ngx_http_postpone_filter_module */
                        r->connection->data = r->main;
                        rc = ngx_http_output_filter(r->main, &out);
                        r->connection->data = ar;
                    } else {
                        rc = ngx_http_output_filter(r->main, &out);
                    }

                    if (rc == NGX_ERROR) {
                        xrltStringClear(&s);
                        return NGX_ERROR;
                    }
                }

                xrltStringClear(&s);
            }
        }
    }

    if (result & XRLT_STATUS_LOG) {
        xrltLogType   ltype;
        ngx_uint_t    l;

        while (xrltLogListShift(&ctx->xctx->log, &ltype, &s)) {
            switch (ltype) {
                case XRLT_ERROR:   l = NGX_LOG_ERR; break;
                case XRLT_WARNING: l = NGX_LOG_WARN; break;
                case XRLT_DEBUG:   l = NGX_LOG_DEBUG; break;
                case XRLT_INFO:
                default:           l = NGX_LOG_INFO; break;
            }

            ngx_log_error(l, r->connection->log, 0, s.data);

            xrltStringClear(&s);
        }
    }

    if (result & XRLT_STATUS_DONE) {
        if (!ctx->main_ctx->headers_sent) {
            ctx->main_ctx->headers_sent = 1;

            dd("Sending response headers");

            if (ngx_http_send_header(r->main) != NGX_OK) {
                return NGX_ERROR;
            }
        }

        r->main->write_event_handler = ngx_http_xrlt_wev_handler;

        if (ngx_http_send_special(r->main, NGX_HTTP_LAST) == NGX_ERROR) {
            return NGX_ERROR;
        }

        return NGX_DONE;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_transform_body(ngx_http_request_t *r, ngx_http_xrlt_ctx_t *ctx,
                             size_t id, xrltString *val, xrltBool last,
                             xrltBool error)
{
    xrltTransformValue   v;
    int                  result;
    ngx_int_t            rc;

    dd("Transform body (r: %p, id: %zd, val: %p, last: %d, error: %d)",
       r, id, val, last, error);

    v.type = error ? XRLT_TRANSFORM_VALUE_ERROR : XRLT_TRANSFORM_VALUE_BODY;

    if (val == NULL) {
        ngx_memset(&v.bodyval.val, 0, sizeof(xrltString));
    } else {
        ngx_memcpy(&v.bodyval.val, val, sizeof(xrltString));
    }

    v.bodyval.last = last;

    result = xrltTransform(ctx->xctx, id, &v);

    rc = ngx_http_xrlt_process_transform_result(r, ctx, result);

    if (rc == NGX_OK) {
        v.type = XRLT_TRANSFORM_VALUE_EMPTY;

        while (rc == NGX_OK) {
            result = xrltTransform(ctx->xctx, id, &v);

            rc = ngx_http_xrlt_process_transform_result(r, ctx, result);
        }
    }

    return rc == NGX_ERROR ? NGX_ERROR : NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_post_sr(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_xrlt_ctx_t  *sr_ctx = data;
    xrltBool              error;

    if (sr_ctx->run_post_subrequest) {
        if (r != r->connection->data) {
            r->connection->data = r;
        }

        return NGX_OK;
    }

    sr_ctx->run_post_subrequest = 1;

    if (!sr_ctx->refused) {
        error = rc == NGX_ERROR ? TRUE : FALSE;

        if (ngx_http_xrlt_transform_body(r, sr_ctx, sr_ctx->id, NULL,
                                         TRUE, error) == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    if (r != r->connection->data) {
        r->connection->data = r;
    }

    dd("Subrequest completed");

    return NGX_OK;
}


static ngx_int_t
ngx_http_xrlt_transform_headers(ngx_http_request_t *r, ngx_http_xrlt_ctx_t *ctx)
{
    ngx_list_part_t     *part;
    ngx_table_elt_t     *header;
    ngx_uint_t           i;
    ngx_int_t            rc;
    xrltTransformValue   val;
    int                  result;
    ngx_table_elt_t    **cookies;
    char                *begin, *end;

    dd("Transform headers (r: %p, id: %zd)", r, ctx->id);

    if (r == r->main) {
        part = &r->headers_in.headers.part;

        cookies = r->headers_in.cookies.elts;

        val.type = XRLT_TRANSFORM_VALUE_COOKIE;

        for (i = 0; i < r->headers_in.cookies.nelts; i++) {
            begin = (char *)cookies[i]->value.data;
            end = (char *)cookies[i]->value.data + cookies[i]->value.len;

            while (begin < end) {
                while (begin < end && *begin == ' ') {
                    begin++;
                }

                if (begin >= end) {
                    break; // Ignore incorrect cookie header.
                }

                val.headerval.name.data = begin;
                val.headerval.name.len = 0;

                while (begin < end && *begin != '=') {
                    begin++;
                }

                if (begin < end && val.headerval.name.data < begin) {
                    val.headerval.name.len = begin - val.headerval.name.data;
                    begin++;

                    val.headerval.val.data = begin;
                } else {
                    break; // Ignore incorrect cookie header.
                }

                while (begin < end && *begin != ';') {
                    begin++;
                }

                val.headerval.val.len = begin - val.headerval.val.data;

                if (val.headerval.val.len == 0) {
                    val.headerval.val.data = NULL;
                }

                result = xrltTransform(ctx->xctx, 0, &val);

                rc = ngx_http_xrlt_process_transform_result(r, ctx, result);

                if (rc == NGX_ERROR || rc == NGX_DONE) {
                    return rc;
                }

                begin++;
            }
        }

        val.type = XRLT_TRANSFORM_VALUE_QUERYSTRING;

        val.querystringval.val.data = (char *)r->args.data;
        val.querystringval.val.len = r->args.len;

        result = xrltTransform(ctx->xctx, 0, &val);

        rc = ngx_http_xrlt_process_transform_result(r, ctx, result);

        if (rc == NGX_ERROR || rc == NGX_DONE) {
            return rc;
        }
    } else {
        part = &r->headers_out.headers.part;
    }

    header = part->elts;

    val.type = XRLT_TRANSFORM_VALUE_HEADER;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        val.headerval.name.data = (char *)header[i].key.data;
        val.headerval.name.len = header[i].key.len;

        val.headerval.val.data = (char *)header[i].value.data;
        val.headerval.val.len = header[i].value.len;

        result = xrltTransform(ctx->xctx, ctx->id, &val);

        rc = ngx_http_xrlt_process_transform_result(r, ctx, result);

        if (rc == NGX_ERROR || rc == NGX_DONE) {
            return rc;
        }
    }

    if (r != r->main) {
        val.type = XRLT_TRANSFORM_VALUE_STATUS;

        val.statusval.status = r->headers_out.status;

        result = xrltTransform(ctx->xctx, ctx->id, &val);

        rc = ngx_http_xrlt_process_transform_result(r, ctx, result);

        if (rc == NGX_ERROR || rc == NGX_DONE) {
            return rc;
        }
    }

    val.type = XRLT_TRANSFORM_VALUE_EMPTY;
    rc = NGX_OK;

    while (rc == NGX_OK) {
        result = xrltTransform(ctx->xctx, 0, &val);

        rc = ngx_http_xrlt_process_transform_result(r, ctx, result);
    }

    return rc;
}


static ngx_int_t
ngx_http_xrlt_header_filter(ngx_http_request_t *r)
{
    ngx_http_xrlt_ctx_t         *ctx;

    if (r == r->main) {
        return ngx_http_next_header_filter(r);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    if (ctx->refused || ngx_http_xrlt_transform_headers(r, ctx) != NGX_ERROR) {
        return NGX_OK;
    } else {
        return NGX_ERROR;
    }
}


static ngx_int_t
ngx_http_xrlt_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_xrlt_ctx_t  *ctx;
    ngx_chain_t          *cl;
    ngx_int_t             rc;
    xrltString            s;

    if (r == r->main) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    for (cl = in; cl; cl = cl->next) {
        s.data = (char *)cl->buf->pos;
        s.len = cl->buf->last - cl->buf->pos;

        if (!ctx->refused) {
            rc = ngx_http_xrlt_transform_body(r, ctx, ctx->id, &s, FALSE,
                                              FALSE);

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }
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



static void
ngx_http_xrlt_post_request_body(ngx_http_request_t *r)
{
    ngx_http_xrlt_ctx_t  *ctx;
    ngx_chain_t          *cl;
    ngx_int_t             rc;
    xrltString            s;

    if (r != r->main) { return; }

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);

    dd("Post request body (r: %p)", r);

    if (r->request_body->bufs == NULL) {
        s.data = NULL;
        s.len = 0;

        rc = ngx_http_xrlt_transform_body(r, ctx, 0, &s, TRUE, FALSE);

        if (rc == NGX_ERROR) {
            ngx_http_finalize_request(r, NGX_ERROR);

            return;
        }
    } else {
        for (cl = r->request_body->bufs; cl; cl = cl->next) {
            s.data = (char *)cl->buf->pos;
            s.len = cl->buf->last - cl->buf->pos;

            rc = ngx_http_xrlt_transform_body(r, ctx, 0, &s, cl->buf->last_buf,
                                              FALSE);

            if (rc == NGX_ERROR) {
                ngx_http_finalize_request(r, NGX_ERROR);

                return;
            }

            cl->buf->pos = cl->buf->last;
            cl->buf->file_pos = cl->buf->file_last;
        }
    }
}


static ngx_int_t
ngx_http_xrlt_handler(ngx_http_request_t *r) {
    ngx_http_xrlt_ctx_t  *ctx;
    ngx_int_t             rc;

    dd("XRLT begin (main: %p)", r->main);

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (ctx == NULL) {
        ctx = ngx_http_xrlt_create_ctx(r, 0);
        if (ctx == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_xrlt_module);
    }

    r->headers_out.status = NGX_HTTP_OK;

    rc = ngx_http_xrlt_transform_headers(r, ctx);
    if (rc == NGX_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (rc == NGX_DONE) {
        return NGX_OK;
    }

    if (ctx->xctx->bodyData != NULL) {
        rc = ngx_http_read_client_request_body(
            r, ngx_http_xrlt_post_request_body
        );

        if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
        }

        if (ctx->xctx->cur & XRLT_STATUS_DONE) {
            ngx_http_xrlt_wev_handler(r);

            return NGX_OK;
        }
    } else {
        r->main->count++;
    }

    return NGX_DONE;
}


static void
ngx_http_xrlt_cleanup_requestsheet(void *data)
{
    dd("XRLT requestsheet cleanup");

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


    param->name.data = ngx_pstrdup(cf->pool, &value[1]);
    if (param->name.data == NULL) {
        return NGX_CONF_ERROR;
    }

    param->name.len = value[1].len;

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
    ngx_http_xrlt_param_t     *prevparams, *params, *param;
    ngx_uint_t                 i, j;
    xrltBool                   add;

    if (conf->params == NULL) {
        conf->params = prev->params;
    } else if (prev->params != NULL) {
        prevparams = prev->params->elts;
        params = conf->params->elts;

        for (i = 0; i < prev->params->nelts; i++) {
            add = TRUE;

            for (j = 0; j < conf->params->nelts; j++) {
                if (prevparams[i].name.len != params[j].name.len ||
                    ngx_strncmp(prevparams[i].name.data, params[j].name.data,
                                params[j].name.len) == 0)
                {
                    add = FALSE;
                    break;
                }
            }

            if (add) {
                param = ngx_array_push(conf->params);
                if (param == NULL) {
                    return NGX_CONF_ERROR;
                }

                param->name = prevparams[i].name;
                param->value = prevparams[i].value;
            }
        }
    }

    return NGX_CONF_OK;
}


static void
ngx_http_xrlt_exit(ngx_cycle_t *cycle)
{
    //xsltCleanupGlobals();
    xmlCleanupParser();
}
