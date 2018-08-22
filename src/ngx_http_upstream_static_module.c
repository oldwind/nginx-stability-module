/*
 * Copyright (C) yebin
 */

// #ifndef _NGX_HTTP_STABLITY_
// #define _NGX_HTTP_STABLITY_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static void * 
ngx_http_upstream_static_init_root();

static char * ngx_http_stablity_rule(ngx_conf_t *cf, ngx_command_t *cmd, 
    void *conf);

static ngx_int_t ngx_http_stablity_init(ngx_conf_t *cf);

static void * 
ngx_http_stablity_create_loc_conf(ngx_conf_t *cf);

static char * 
ngx_http_stablity_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t
ngx_http_static_handler(ngx_http_request_t *r);


static ngx_command_t  ngx_http_upstream_static_module_commands[] = {

    // 将upstream静态化缓存保存地址
    // { ngx_string("upstream_static_root"),
    //     NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    //     ngx_http_stablity_rule,
    //     NGX_HTTP_LOC_CONF_OFFSET,
    //     0,
    //     NULL },

    // 将upstream内容静态化的内容大小，超过限制，不做静态化
    { ngx_string("upstream_static_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_stablity_rule,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    // upstream内容保存有效时间
    { ngx_string("upstream_static_active_time"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_stablity_rule,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },    

    ngx_null_command
};

static ngx_http_module_t  ngx_http_upstream_static_module_ctx = {
    ngx_http_upstream_static_init_root,      /* preconfiguration */
    ngx_http_stablity_init,                  /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_stablity_create_loc_conf,       /* create location configuration */
    ngx_http_stablity_merge_loc_conf         /* merge location configuration */
};


ngx_module_t  ngx_http_upstream_static_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_static_module_ctx,           /* module context */
    ngx_http_upstream_static_module_commands,       /* module directives */
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

// 初始化缓存文件的地址，判断文件夹是否存在，不存在，则新建
static void *
ngx_http_upstream_static_init_root() {


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