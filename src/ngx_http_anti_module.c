/*
 * Copyright (C) yebin
 */

// #ifndef _ngx_http_anti_
// #define _ngx_http_anti_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

// 计数器的hash结构，开链方式，val统一是int类型
typedef struct anti_hash_t{ 
    ngx_int_t expire_tm;
    ngx_str_t key;
    ngx_int_t val;
    struct anti_hash_t * next;
} anti_hash_t;


typedef struct {
    ngx_int_t       anti_cache_type;
    ngx_int_t       anti_cache_key;
    ngx_int_t       anti_hash_size;
    ngx_int_t       anti_frozen_time;
    ngx_int_t       anti_use_redis;
    ngx_int_t       anti_redis_address;
    ngx_int_t       anti_redis_connect_keeplive;
    anti_hash_t     **anti_hash;
    ngx_shm_zone_t  *shm_zone;
} ngx_http_anti_conf_t;


static ngx_int_t 
    ngx_http_anti_preconf_init(ngx_conf_t *cf);

static ngx_int_t 
    ngx_http_anti_postconf_init(ngx_conf_t *cf);

static void * 
    ngx_http_anti_create_loc_conf(ngx_conf_t *cf);

static char * 
    ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t 
    ngx_http_anti_handler(ngx_http_request_t *r);

static anti_hash_t **
    anti_hash_tbl_create(ngx_conf_t *cf, ngx_int_t hash_size);

static ngx_int_t 
    anti_hash_tbl_find(anti_hash_t  ** anti_hash, ngx_str_t * str, ngx_int_t hash_size);

static ngx_int_t 
    anti_hash_tbl_set(ngx_http_anti_conf_t *ancf,  ngx_str_t * str,  ngx_int_t val, ngx_int_t expire_tm); 

// static ngx_int_t 
//     anti_hash_tbl_del(anti_hash_t  ** anti_hash, ngx_str_t * str, ngx_int_t hash_size);


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

    { ngx_string("anti_hash_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_hash_size),
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
    ngx_http_anti_preconf_init,      /* preconfiguration */
    ngx_http_anti_postconf_init,                  /* postconfiguration */

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
    ngx_str_t key = ngx_string("sss");
    ngx_int_t len = key.len + ngx_strlen(uri);

    char str[100];
    // sprintf(str, "ip=%d", (int)s_addr);   
    // ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, str);

    if (len <= 0) {
        return NGX_OK;
    }

    // 
    anti_hash_t ** hash_header = ancf->anti_hash;
    ngx_int_t hash_size   = 100;

    // 测试hashtable set
    ngx_str_t test_str = ngx_string("/sisisisis?sdajfda");
    
    // 测试hashtable find
    ngx_int_t num = anti_hash_tbl_find(hash_header, &test_str, hash_size);

    num ++;

    anti_hash_tbl_set(ancf, &test_str, num, 12345678);

    sprintf(str, "request_tm=%ld", num);
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, str);



    // 写到共享内存

    

    return NGX_OK;
}


// 生成hash表，存储key->value, 开链法
// @todo rehash
static anti_hash_t **
anti_hash_tbl_create(ngx_conf_t *cf, ngx_int_t hash_size) {
    
    anti_hash_t ** anti_hash;
   
    anti_hash = (anti_hash_t **)ngx_pcalloc(cf->pool, sizeof(anti_hash_t *) * hash_size);
    if (anti_hash == NULL) {
        return NULL;
    }

    return anti_hash;
}

//
// 从cache里面查到string是否存在, 查的过程中，发现如果链表数据过期，将数据删除
// hash算法选择mermerhash算法
static ngx_int_t 
anti_hash_tbl_find(anti_hash_t ** hash_header, ngx_str_t * find_str, ngx_int_t hash_size) {
    
    uint32_t hash_pos = ngx_murmur_hash2(find_str->data, find_str->len);
   
    anti_hash_t *hash_tmp;
    
    hash_tmp = hash_header[hash_pos % hash_size];
    
    while (hash_tmp != NULL ) {

        if (ngx_strcmp(hash_tmp->key.data, find_str->data)) {
            return hash_tmp->val;
        }

        hash_tmp = hash_tmp->next;
    }

    return 0;
} 

// 将str值放在hash表的最后位置
static ngx_int_t 
anti_hash_tbl_set(ngx_http_anti_conf_t *ancf,  ngx_str_t * str,  ngx_int_t val, ngx_int_t expire_tm) {

    ngx_slab_pool_t      *shpool;

    anti_hash_t          *cache_info;

    anti_hash_t **anti_hash = ancf->anti_hash;
    ngx_int_t hash_size     = ancf->anti_hash_size;
    uint32_t hash_pos       = ngx_murmur_hash2(str->data, str->len);

    // 将str值放在共享内存中
    ngx_int_t size = sizeof(anti_hash_t) + str->len;
    
    // 共享内存
    shpool = (ngx_slab_pool_t *) ancf->shm_zone->shm.addr;

    // 加锁
    ngx_shmtx_lock(&shpool->mutex);
   
    // 通过slab分配内存
    cache_info = ngx_slab_alloc_locked(shpool, size);

    // 分配失败，解锁返回错误
    if (cache_info == NULL) {
        ngx_shmtx_unlock(&shpool->mutex);
        return NGX_ERROR;
    }

    cache_info->next      = NULL;
    cache_info->val       = val;
    cache_info->expire_tm = expire_tm;

    cache_info->key.len  = str->len;
    cache_info->key.data = (u_char * ) (cache_info + sizeof(anti_hash_t));

    ngx_memcpy(cache_info->key.data, str->data, str->len);

    anti_hash_t * temp_hash = anti_hash[hash_pos % hash_size];
    
    if (temp_hash == NULL) {
        temp_hash = cache_info;
    } else {
        while (temp_hash->next != NULL) {
            temp_hash = temp_hash->next;
        }
        temp_hash->next = cache_info;
    }
    
    ngx_shmtx_unlock(&shpool->mutex);
    return NGX_OK;
} 


// static ngx_int_t 
//     anti_hash_tbl_del(cache_key_hash_t * ckh, ngx_str_t * str, ngx_int_t hash_size) 
// {
//     return NGX_OK;
// }



static void * 
ngx_http_anti_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_anti_conf_t *ancf;
    ancf = ngx_pcalloc(cf->pool, sizeof(ngx_http_anti_module));
    if (ancf == NULL) {
        return NULL;
    }

    ancf->anti_cache_type       = NGX_CONF_UNSET_UINT;
    ancf->anti_cache_key        = NGX_CONF_UNSET_UINT;
    ancf->anti_hash_size        = NGX_CONF_UNSET_UINT;
    ancf->anti_frozen_time      = NGX_CONF_UNSET_UINT;
    ancf->anti_use_redis        = NGX_CONF_UNSET_UINT; 
    ancf->anti_redis_address    = NGX_CONF_UNSET_UINT;
    ancf->anti_redis_connect_keeplive = NGX_CONF_UNSET_UINT;
    ancf->anti_hash             = NULL;

    ngx_int_t hash_size = 100;
    ancf->anti_hash = anti_hash_tbl_create(cf, hash_size);

    ngx_shm_zone_t   *shm_zone;
    ngx_str_t name = ngx_string("anti_hash");
    shm_zone = ngx_shared_memory_add(cf, &name, 1024,
                                     &ngx_http_anti_module);

    ancf->shm_zone = shm_zone;

    return ancf;
}

 
static char * 
ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_anti_conf_t *prev = parent;
    ngx_http_anti_conf_t *conf = child;

    ngx_conf_merge_value(conf->anti_cache_type, prev->anti_cache_type, 1);
    ngx_conf_merge_value(conf->anti_cache_key, prev->anti_cache_key, 12);
    ngx_conf_merge_value(conf->anti_hash_size, prev->anti_hash_size, 100);
    ngx_conf_merge_value(conf->anti_frozen_time, prev->anti_frozen_time, 300);
    ngx_conf_merge_value(conf->anti_use_redis, prev->anti_use_redis, 1);
    ngx_conf_merge_value(conf->anti_redis_address, prev->anti_redis_address, 1);
    ngx_conf_merge_value(conf->anti_redis_connect_keeplive, prev->anti_redis_connect_keeplive, 1);

    return NGX_CONF_OK;
}


// 初始化缓存文件的地址，判断文件夹是否存在，不存在，则新建
// 初始化hash表
static ngx_int_t
ngx_http_anti_preconf_init(ngx_conf_t *cf) {
    // ngx_http_anti_conf_t * ancf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_anti_module);

    

    return NGX_OK;
}


static ngx_int_t 
ngx_http_anti_postconf_init(ngx_conf_t *cf) {
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