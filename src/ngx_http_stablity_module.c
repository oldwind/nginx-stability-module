/*
 * Copyright (C) yebin
 */

// #ifndef _NGX_HTTP_STABLITY_
// #define _NGX_HTTP_STABLITY_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char * ngx_http_stablity_rule(ngx_conf_t *cf, ngx_command_t *cmd, 
    void *conf);


static ngx_int_t ngx_http_stablity_init(ngx_conf_t *cf);

static void * ngx_http_stablity_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_stablity_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t
ngx_http_static_handler(ngx_http_request_t *r);

static ngx_command_t  ngx_http_stablity_commands[] = {

    { ngx_string("stablity_cache"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_stablity_rule,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

      ngx_null_command
};



static ngx_http_module_t  ngx_http_stablity_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_stablity_init,                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_stablity_create_loc_conf,       /* create location configuration */
    ngx_http_stablity_merge_loc_conf         /* merge location configuration */
};


ngx_module_t  ngx_http_stablity_module = {
    NGX_MODULE_V1,
    &ngx_http_stablity_module_ctx,           /* module context */
    ngx_http_stablity_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_static_handler(ngx_http_request_t *r)
{
    return NGX_OK;
}

static void * 
ngx_http_stablity_create_loc_conf(ngx_conf_t *cf) {
    return NGX_OK;
}


static char * 
ngx_http_stablity_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    return NGX_OK;
}

static char * 
ngx_http_stablity_rule(ngx_conf_t *cf, ngx_command_t *cmd,  void *conf) {
    return "";
}

static ngx_int_t 
ngx_http_stablity_init(ngx_conf_t *cf) {
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_static_handler;

    return NGX_OK; 
}