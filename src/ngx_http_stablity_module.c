
/*
 * Copyright (C) yebin
 */


#ifndef _NGX_HTTP_STABLITY_
#define _NGX_HTTP_STABLITY_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_command_t  ngx_http_access_commands[] = {

    { ngx_string("stablity_cache"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_access_rule,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    { ngx_string("stablity_cache"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_access_rule,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

      ngx_null_command
};



static ngx_http_module_t  ngx_http_stablity_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_access_init,                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_access_create_loc_conf,       /* create location configuration */
    ngx_http_access_merge_loc_conf         /* merge location configuration */
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