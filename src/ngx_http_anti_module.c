/*
 * Copyright (C) yebin
 */

// #ifndef _ngx_http_anti_
// #define _ngx_http_anti_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_anti_init_root(ngx_conf_t *cf);

static ngx_int_t ngx_http_anti_init(ngx_conf_t *cf);
static void * ngx_http_anti_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_anti_handler(ngx_http_request_t *r);


typedef struct {
    ngx_int_t    anti_cache_type;
    ngx_int_t    anti_cache_key;
    ngx_int_t    anti_frozen_time;
    ngx_int_t    anti_use_redis;
    ngx_int_t    anti_redis_address;
    ngx_int_t    anti_redis_connect_keeplive;
} ngx_http_anti_conf_t;


static ngx_command_t  ngx_http_anti_module_commands[] = {
    
    { ngx_string("anti_cache_type"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_cache_type),
        NULL },
    
    { ngx_string("anti_cache_key"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_cache_key),
        NULL },

    { ngx_string("anti_frozen_time"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_frozen_time),
        NULL },
    
    { ngx_string("anti_use_redis"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_use_redis),
        NULL },

    { ngx_string("anti_redis_address"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_redis_address),
        NULL },

    { ngx_string("anti_redis_connect_keeplive"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_redis_connect_keeplive),
        NULL },    

    ngx_null_command
};

static ngx_http_module_t  ngx_http_anti_module_ctx = {
    ngx_http_anti_init_root,      /* preconfiguration */
    ngx_http_anti_init,                  /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_anti_create_loc_conf,       /* create location configuration */
    ngx_http_anti_merge_loc_conf         /* merge location configuration */
};

     
ngx_module_t  ngx_http_anti_module = {
    NGX_MODULE_V1,
    &ngx_http_anti_module_ctx,           /* module context */
    ngx_http_anti_module_commands,       /* module directives */
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
ngx_http_anti_handler(ngx_http_request_t *r){
    
    // 获取配置信息
    ngx_http_anti_conf_t * ancf;
    ancf = ngx_http_get_module_loc_conf(r, ngx_http_anti_module);

    // 只是针对IPV4的请求访问，UNIX域请求和IPV6不做处理
    if (r->connection->sockaddr->sa_family != AF_INET) {
        return NGX_OK;
    }

    // 获取client ip信息
    struct sockaddr_in *sin;
    
    sin = (struct sockaddr_in *) r->connection->sockaddr;

    ngx_int_t s_addr = sin->sin_addr.s_addr;
    if (s_addr <= 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get client addr error");
        return NGX_OK;
    }

    // 获取request uri信息
    u_char * uri;
    uri = r->uri.data;

    if (uri == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
            "get request uri is error");
    }

    // 整理成key
    // ngx_str_t *key = ;


    // 访问共享内存，判断是否超过限制
    
    
    
    // 写到共享内存

    

    return NGX_OK;
}


static void * 
ngx_http_anti_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_anti_conf_t *ancf;
    ancf = ngx_pcalloc(cf->pool, sizeof(ngx_http_anti_module));
    if (ancf == NULL) {
        return NULL;
    }

    ancf->anti_cache_type       = NGX_CONF_UNSET_UINT;
    ancf->anti_cache_key        = NGX_CONF_UNSET_UINT;
    ancf->anti_frozen_time      = NGX_CONF_UNSET_UINT;
    ancf->anti_use_redis        = NGX_CONF_UNSET_UINT; 
    ancf->anti_redis_address    = NGX_CONF_UNSET_UINT;
    ancf->anti_redis_connect_keeplive = NGX_CONF_UNSET_UINT;
    return ancf;
}

 
static char * 
ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_anti_conf_t *prev = parent;
    ngx_http_anti_conf_t *conf = child;

    ngx_conf_merge_value(conf->anti_cache_type, prev->anti_cache_type, 1);
    ngx_conf_merge_value(conf->anti_cache_key, prev->anti_cache_key, 12);
    ngx_conf_merge_value(conf->anti_frozen_time, prev->anti_frozen_time, 300);
    ngx_conf_merge_value(conf->anti_use_redis, prev->anti_use_redis, 1);
    ngx_conf_merge_value(conf->anti_redis_address, prev->anti_redis_address, 1);
    ngx_conf_merge_value(conf->anti_redis_connect_keeplive, prev->anti_redis_connect_keeplive, 1);

    return NGX_CONF_OK;
}


// 初始化缓存文件的地址，判断文件夹是否存在，不存在，则新建
static ngx_int_t
ngx_http_anti_init_root(ngx_conf_t *cf) {
    return NGX_OK;
}


static ngx_int_t 
ngx_http_anti_init(ngx_conf_t *cf) {
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_anti_handler;

    return NGX_OK; 
}