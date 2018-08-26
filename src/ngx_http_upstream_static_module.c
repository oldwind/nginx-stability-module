/*
 * Copyright (C) yebin
 */

// #ifndef _ngx_http_upstream_static_
// #define _ngx_http_upstream_static_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_upstream_static_init_root(ngx_conf_t *cf);

static ngx_int_t ngx_http_upstream_static_init(ngx_conf_t *cf);
static void * ngx_http_upstream_static_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_upstream_static_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_upstream_static_handler(ngx_http_request_t *r);


typedef struct {
    ngx_int_t          size;
    ngx_int_t          active_time;
} ngx_http_upstream_static_conf_t;


static ngx_command_t  ngx_http_upstream_static_module_commands[] = {

    // 将upstream内容静态化的内容大小，超过限制，不做静态化
    { ngx_string("upstream_static_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_upstream_static_conf_t, size),
        NULL },

    // upstream内容保存有效时间
    { ngx_string("upstream_static_active_time"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_upstream_static_conf_t, active_time),
        NULL },    

    ngx_null_command
};

static ngx_http_module_t  ngx_http_upstream_static_module_ctx = {
    ngx_http_upstream_static_init_root,      /* preconfiguration */
    ngx_http_upstream_static_init,                  /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_upstream_static_create_loc_conf,       /* create location configuration */
    ngx_http_upstream_static_merge_loc_conf         /* merge location configuration */
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
ngx_http_upstream_static_handler(ngx_http_request_t *r){
    
    // 获取配置信息
    // ngx_http_upstream_static_conf_t * uscf;
    //uscf = ngx_http_get_module_loc_conf(r, ngx_http_upstream_static_module);

    printf("hello");


    return NGX_OK;
}


static void * 
ngx_http_upstream_static_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_upstream_static_conf_t *uscf;
    uscf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fastcgi_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    uscf->active_time = NGX_CONF_UNSET_UINT;
    uscf->size        = NGX_CONF_UNSET_UINT;

    return uscf;
}

 
static char * 
ngx_http_upstream_static_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    


    return NGX_OK;
}



// 初始化缓存文件的地址，判断文件夹是否存在，不存在，则新建
static ngx_int_t
ngx_http_upstream_static_init_root(ngx_conf_t *cf) {
    return NGX_OK;
}


static ngx_int_t 
ngx_http_upstream_static_init(ngx_conf_t *cf) {
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_upstream_static_handler;

    return NGX_OK; 
}