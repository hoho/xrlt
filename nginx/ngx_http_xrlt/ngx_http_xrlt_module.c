#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define DDEBUG 1
#include "ddebug.h"


static ngx_int_t ngx_http_xrlt_filter_init(ngx_conf_t *cf);
static char *ngx_http_xrlt(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_command_t ngx_http_xrlt_commands[] = {
        { ngx_string("xrlt"),
          NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
          ngx_http_xrlt,
          NGX_HTTP_LOC_CONF_OFFSET,
          0,
          NULL },
        ngx_null_command
};


ngx_http_module_t ngx_http_xrlt_module_ctx = {
        NULL,                             /*  preconfiguration */
        ngx_http_xrlt_filter_init,        /*  postconfiguration */

        NULL,                             /*  create main configuration */
        NULL,                             /*  init main configuration */

        NULL,                             /*  create server configuration */
        NULL,                             /*  merge server configuration */

        NULL,    /*  create location configuration */
        NULL                              /*  merge location configuration */
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
        NULL,                       /*  exit process */
        NULL,                       /*  exit master */
        NGX_MODULE_V1_PADDING
};


typedef struct ngx_http_xrlt_ctx_s {
    ngx_int_t data;
} ngx_http_xrlt_ctx_t;


ngx_inline static ngx_http_xrlt_ctx_t *
ngx_http_xrlt_create_ctx(ngx_http_request_t *r) {
    ngx_http_xrlt_ctx_t      *ctx;

    ctx = ngx_palloc(r->pool, sizeof(ngx_http_xrlt_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    return ctx;
}


static ngx_int_t
ngx_http_xrlt_subrequest(ngx_http_request_t *r, ngx_http_xrlt_ctx_t *ctx,
        const char *uri, ngx_int_t data)
{
    ngx_http_xrlt_ctx_t         *sr_ctx;
    ngx_str_t                    sr_uri;
    ngx_http_request_t          *sr;

    sr_ctx = ngx_http_xrlt_create_ctx(r);
    if (sr_ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    sr_ctx->data = data;

    sr_uri.data = uri;
    sr_uri.len = strlen(uri);

    if (ngx_http_subrequest(r, &sr_uri, &sr_uri, &sr, NULL, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    ngx_http_set_ctx(sr, sr_ctx, ngx_http_xrlt_module);

    return NGX_OK;
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
    ngx_http_xrlt_ctx_t         *ctx;
    ngx_chain_t                 *cl;
    ngx_buf_t                   *b;
    ngx_chain_t                  out;

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (in == NULL || r == r->main || ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    //dd("BOFIFI %s", in->buf->pos);

    dd("BOOOODYYY %d", ctx->data);


    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    if (b == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }


    b->last_buf = 0; /* there will be no more buffers in the request */

    if (ctx->data == 1) {
        dd("BAAAA: %d, %p", in->buf->end - in->buf->start, r->main);
        b->start = b->pos = "yyyy\n"; /* first position in memory of the data */
        b->end = b->last = "yyyy\n" + 5; /* last position */
    } else if (ctx->data == 2) {
        dd("BOOOOO: %d, %p", in->buf->end - in->buf->start, r->main);
        b->start = b->pos = "zzzz\n"; /* first position in memory of the data */
        b->end = b->last = "zzzz\n" + 5; /* last position */

        if (in->buf->last_in_chain) {
            if (ngx_http_xrlt_subrequest(r->main, ctx, "/sub2", 3) != NGX_OK) {
                return NGX_ERROR;
            }
            dd("YOYOYO!!!!!");
        }
    } else if (ctx->data == 4) {
        dd("BRRRRR: %d, %p", in->buf->end - in->buf->start, r->main);
        b->start = b->pos = "uuuu\n";
        b->end = b->last = "uuuu\n" + 5;
    } else {
        dd("BFFFF: %d, %p, %s, %d", in->buf->end - in->buf->start, r->main, in->buf->start, in->buf->last_in_chain);
        b->start = b->pos = "iiii\n"; /* first position in memory of the data */
        b->end = b->last = "iiii\n" + 5; /* last position */

        b->last_buf = 1; /* there will be no more buffers in the request */

        //if (in->buf->last_in_chain) {
        //    ngx_http_send_special(r->main, NGX_HTTP_LAST);
        //}
    }

    b->memory = 1; /* content is in read-only memory */
    /* (i.e., filters should copy it rather than rewrite in place) */

    //b->last_buf = 1; /* there will be no more buffers in the request */

    b->last_in_chain = 1;
    b->flush = 1;

    out.buf = b;
    out.next = NULL;

    for (cl = in; cl; cl = cl->next) {
        cl->buf->pos = cl->buf->last;
        cl->buf->file_pos = cl->buf->file_last;
    }

    return ngx_http_output_filter(r->main, &out);
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

    ngx_http_xrlt_ctx_t         *ctx;
    ngx_chain_t                  out;
    ngx_buf_t                   *b;

    ctx = ngx_http_get_module_ctx(r, ngx_http_xrlt_module);
    if (ctx == NULL) {
        ctx = ngx_http_xrlt_create_ctx(r);
        if (ctx == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_xrlt_module);
    }

    r->headers_out.status = NGX_HTTP_OK;
    //r->headers_out.content_length_n = 100;
    ngx_http_send_header(r);


    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    if (b == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->pos = "alal\n"; /* first position in memory of the data */
    b->last = "alal\n" + 5; /* last position */

    b->memory = 1; /* content is in read-only memory */
    /* (i.e., filters should copy it rather than rewrite in place) */

    b->last_buf = 0; /* there will be no more buffers in the request */
    b->last_in_chain = 1;
    b->flush = 1;
    b->temporary = 1;

    out.buf = b;
    out.next = NULL;


    if (ngx_http_xrlt_subrequest(r, ctx, "/CHANGES", 1) != NGX_OK) {
        return NGX_ERROR;
    }

    if (ngx_http_xrlt_subrequest(r, ctx, "/sub", 2) != NGX_OK) {
        return NGX_ERROR;
    }

    if (ngx_http_xrlt_subrequest(r, ctx, "/sub", 4) != NGX_OK) {
        return NGX_ERROR;
    }

    return ngx_http_output_filter(r, &out);

    return NGX_OK;
}


static char *
ngx_http_xrlt(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t    *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (clcf == NULL) {
        return NGX_CONF_ERROR;
    }

    clcf->handler = ngx_http_xrlt_handler;

    return NGX_CONF_OK;
}
