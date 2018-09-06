/*
 * Copyright (C) yebin
 */

// #ifndef _ngx_http_anti_
// #define _ngx_http_anti_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


// 实现一个hashtable
typedef struct anti_hash_t {
    ngx_str_t hash_key;
    void *    hash_val;
    struct anti_hash_t * next;
} anti_hash_t;

// 冻结数据结构
typedef struct {
    ngx_int_t count;
    ngx_int_t expire_tm;
}anti_hash_frozen_t;

// 数据采集结构
typedef struct  {
    ngx_int_t count;
}anti_hash_acqu_t;


// 基础conf信息
typedef struct {
    // command解析出的参数
    ngx_int_t       anti_open;
    ngx_int_t       anti_shm_size;
    ngx_int_t       anti_acqu_cycle;
    ngx_int_t       anti_acqu_type;
    ngx_int_t       anti_threshold;
    ngx_int_t       anti_frozen_innernet;
    ngx_int_t       anti_frozen_time;
    ngx_int_t       anti_acqu_hash_size;
    ngx_int_t       anti_frozen_hash_size; 
    
    ngx_int_t  begin_tm;  // 采集周期开始时间

    anti_hash_t     **anti_acqu_hash;
    ngx_shm_zone_t  *anti_shm_zone;  // 共享内存，初始化为NULL
 
    anti_hash_t     **anti_frozen_hash;  // 冻结地址信息记录

} ngx_http_anti_conf_t;


static ngx_int_t
anti_hash_set(ngx_slab_pool_t * shpool,  anti_hash_t **header, ngx_int_t hash_size, 
    ngx_str_t *key, void *val, ngx_int_t len);

static void * 
anti_hash_find(anti_hash_t **header, ngx_str_t * find_str, ngx_int_t hash_size);


static ngx_int_t 
anti_hash_del(ngx_http_anti_conf_t *ancf, void * dp); 

static ngx_int_t 
anti_shm_init(ngx_shm_zone_t *zone, void *data);

static char * ngx_anti_shm_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_anti_acqu_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_anti_frozen_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t  ngx_http_anti_postconf_init(ngx_conf_t *cf);
static void *  ngx_http_anti_create_loc_conf(ngx_conf_t *cf);
static char *  ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_anti_handler(ngx_http_request_t *r);


static ngx_command_t  ngx_http_anti_module_commands[] = {

    // 指令解析的时候，增加共享内存的大小分配
    // 共享内存，存两部分数据，一部分是记录address的请求数据计数， 二部分记录过期的数据
    { ngx_string("anti_open"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_open),
        NULL },

    // 指令解析的时候，增加共享内存的大小分配
    // 共享内存，存两部分数据，一部分是记录address的请求数据计数， 二部分记录过期的数据
    { ngx_string("anti_shm_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_anti_shm_size_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_shm_size),
        NULL },

    // 设定数据采集周期，单位分，采集周期通过queue的形式存储，queue下面挂这请求的hash table，每分钟清理一次采集内存
    { ngx_string("anti_acqu_cycle"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_acqu_cycle),
        NULL },

    // 数据采集类型，IP + URL， IP， 参数？@todo 慢慢支持
    { ngx_string("anti_acqu_type"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_acqu_type),
        NULL },

    // 采集周期内超过多少次，启动冻结功能
    { ngx_string("anti_threshold"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_threshold),
        NULL },

    // 是否支持冻结内网IP 
    { ngx_string("anti_frozen_innernet"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_frozen_innernet),
        NULL },

    // 冻结时长
    { ngx_string("anti_frozen_time"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_frozen_time),
        NULL },
    
    // 设定采集数据存储hash table的数组长度
    { ngx_string("anti_acqu_hash_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_anti_acqu_hash_size_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_acqu_hash_size),
        NULL },

    // 冻结数据hash table的数组长度
    { ngx_string("anti_frozen_hash_size"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_anti_frozen_hash_size_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_anti_conf_t, anti_frozen_hash_size),
        NULL },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_anti_module_ctx = {
    NULL,          /* preconfiguration */
    ngx_http_anti_postconf_init,         /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_anti_create_loc_conf,       /* create location configuration */
    ngx_http_anti_merge_loc_conf         /* merge location configuration */
};

     
ngx_module_t  ngx_http_anti_module = {
    NGX_MODULE_V1,
    &ngx_http_anti_module_ctx,             /* module context */
    ngx_http_anti_module_commands,         /* module directives */
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
    
    // 1. 系统操作， 获取内存中的当前时间 
    anti_hash_acqu_t hash_acqu     = {1};
    anti_hash_acqu_t *tmp          = NULL;
    anti_hash_frozen_t hash_frozen = {1, 0};
    ngx_http_anti_conf_t *ancf;
    ngx_time_t           *now;
    struct sockaddr_in   *sin;
    char  *client_ip;
    ngx_int_t s_addr;
    u_char *uri;


    ngx_timezone_update();
    now = ngx_timeofday();
    ngx_time_update();
    
    ancf = ngx_http_get_module_loc_conf(r, ngx_http_anti_module); // 获取配置信息

    if (ancf->anti_acqu_hash == NULL) {
        return NGX_OK;
    }

    // 2. 根据类别组装查询的KEY
    if (r->connection->sockaddr->sa_family != AF_INET) { // 只是针对IPV4的请求访问，UNIX域请求和IPV6不做处理
        return NGX_OK;
    }

    sin        = (struct sockaddr_in *) r->connection->sockaddr;
    s_addr     = sin->sin_addr.s_addr;
    client_ip  = inet_ntoa(sin->sin_addr);

    if (s_addr <= 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get client addr error");
        return NGX_OK;
    }

    uri = r->uri.data;  // 获取request uri信息
    if (uri == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get request uri is error");
        return NGX_OK; 
    }

    // 2.1 判断是否是内网IP

    //拼装参数
    char req_str[2048];
    ngx_int_t client_ip_len = ngx_strlen(client_ip);
    ngx_memcpy(req_str, client_ip, client_ip_len);
    ngx_memcpy(req_str + client_ip_len, uri, r->uri.len);

    ngx_str_t request_str; // 测试使用请求参数
    request_str.len  = client_ip_len +  r->uri.len;
    request_str.data = (u_char *)req_str;

    // 3. 判断请求是否被冻结
    anti_hash_t *frozen_base = anti_hash_find(ancf->anti_frozen_hash, &request_str, ancf->anti_frozen_hash_size);
    if (frozen_base != NULL) {
        if ( ((anti_hash_frozen_t *) frozen_base->hash_val)->expire_tm > now->sec ) { // 已过期
            return NGX_HTTP_FORBIDDEN; 
        }
    }


    // 5.1 判断采集开始周期，超过周期，清空内存
    //     改成pool方式，清空pool即可， 不可以在cf的pool中处理
    anti_hash_t  *tmp1, *tmp2;
    if (ancf->begin_tm > 0 ) {
        if (now->sec - ancf->begin_tm > ancf->anti_acqu_cycle) {
            ancf->begin_tm = now->sec;
            for (int i = 0; i < ancf->anti_acqu_hash_size; i ++) {
                if (ancf->anti_acqu_hash[i] == NULL) {
                    continue;
                }
                tmp1 = ancf->anti_acqu_hash[i]->next;
                while (tmp1 != NULL) {
                    tmp2 = tmp1;
                    tmp1 = tmp1->next;
                    anti_hash_del(ancf, tmp2);
                }
                ancf->anti_acqu_hash[i] = NULL;
            }
        }
    } else {
        ancf->begin_tm = now->sec;
    }

    // 6. 查找是否在收集的hashtable中  // 测试hashtable find
    anti_hash_t * hash_temp = anti_hash_find(ancf->anti_acqu_hash, &request_str, ancf->anti_acqu_hash_size);
  
    if (hash_temp != NULL) {
        tmp        = (anti_hash_acqu_t *) hash_temp->hash_val;// 加锁保证原子性
        tmp->count = tmp->count + 1; 

        // 6.1.1 判断是否超过阈值, 写冻结的hashtable中
        if (tmp->count > ancf->anti_threshold) {
            hash_frozen.expire_tm = now->sec + ancf->anti_frozen_time;

            if (frozen_base != NULL) {
                ((anti_hash_frozen_t *) frozen_base->hash_val)->expire_tm = hash_frozen.expire_tm;
                ((anti_hash_frozen_t *) frozen_base->hash_val)->count ++;
           
            } else {
                anti_hash_set((ngx_slab_pool_t *)(ancf->anti_shm_zone->shm.addr), 
                    ancf->anti_frozen_hash, 
                    ancf->anti_frozen_hash_size, 
                    &request_str, 
                    &hash_frozen, 
                    sizeof(anti_hash_frozen_t));
            } 
            return NGX_HTTP_FORBIDDEN;
        }

    } else {
        anti_hash_set((ngx_slab_pool_t *)(ancf->anti_shm_zone->shm.addr), 
                    ancf->anti_acqu_hash, 
                    ancf->anti_acqu_hash_size,
                    &request_str, 
                    &hash_acqu, 
                    sizeof(anti_hash_acqu_t));
    }

    return NGX_OK;
}



static void * 
anti_hash_find(anti_hash_t **header, ngx_str_t * find_str, ngx_int_t hash_size) {
    if (header == NULL) {
        return NULL;
    }

    uint32_t hash_base_size = ngx_murmur_hash2(find_str->data, find_str->len);

    anti_hash_t *hash_temp;
    
    hash_temp = header[hash_base_size % hash_size];

    while (hash_temp != NULL ) {

        if (0 == ngx_strncmp(hash_temp->hash_key.data, find_str->data, hash_temp->hash_key.len)) {
            return hash_temp;
        }

        hash_temp = hash_temp->next;
    }

    return NULL;
}



static ngx_int_t
anti_hash_set(ngx_slab_pool_t * shpool,  anti_hash_t **header, ngx_int_t hash_size, 
    ngx_str_t *key, void *val, ngx_int_t len) 
{
    if (header == NULL || shpool == NULL) {
        return NGX_ERROR;;
    }

    anti_hash_t *anti_hash;
    anti_hash_t *anti_hash_alloc;
    uint32_t hash_base_size;
    
    // 存储大小
    ngx_int_t size = sizeof(anti_hash_t) + key->len + len + 1;

    anti_hash_alloc = ngx_slab_alloc(shpool, size);
    if (anti_hash_alloc == NULL) {
        return NGX_ERROR;
    }

    // 处理key
    anti_hash_alloc->next          = NULL;
    anti_hash_alloc->hash_key.len  = key->len;
    anti_hash_alloc->hash_key.data = (u_char *)anti_hash_alloc + sizeof(anti_hash_t);
    ngx_memcpy(anti_hash_alloc->hash_key.data, key->data, key->len);

    // 处理value
    anti_hash_alloc->hash_val = anti_hash_alloc->hash_key.data + key->len + 1;
    ngx_memcpy(anti_hash_alloc->hash_val, val, len);

    // ngx_int_t anti_acqu_hash_size  = ancf->anti_acqu_hash_size;
    hash_base_size = ngx_murmur_hash2(key->data, key->len);
    anti_hash      = header[hash_base_size % hash_size];

   
    if (anti_hash == NULL) {
        header[hash_base_size % hash_size] = anti_hash_alloc;
    } else {
        while (anti_hash->next != NULL) {
            anti_hash = anti_hash->next;
        }
        anti_hash->next = anti_hash_alloc;
    }
    return NGX_OK;
}


static ngx_int_t 
anti_hash_del(ngx_http_anti_conf_t *ancf, void * dp) 
{
    ngx_slab_pool_t  *shpool;

    // 共享内存池
    shpool = (ngx_slab_pool_t *) ancf->anti_shm_zone->shm.addr;

    ngx_slab_free(shpool, dp);

    return NGX_OK;
}


static void * 
ngx_http_anti_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_anti_conf_t *ancf;
    ancf = ngx_pcalloc(cf->pool, sizeof(ngx_http_anti_module));
    if (ancf == NULL) {
        return NULL;
    }

    ancf->anti_open           = NGX_CONF_UNSET_UINT;
    ancf->anti_shm_size       = NGX_CONF_UNSET;
    ancf->anti_acqu_cycle     = NGX_CONF_UNSET_UINT;
    ancf->anti_acqu_type      = NGX_CONF_UNSET_UINT;
    ancf->anti_threshold      = NGX_CONF_UNSET_UINT;
    ancf->anti_frozen_innernet  = NGX_CONF_UNSET_UINT; 
    ancf->anti_frozen_time      = NGX_CONF_UNSET_UINT;
    ancf->anti_acqu_hash_size   = NGX_CONF_UNSET;
    ancf->anti_frozen_hash_size = NGX_CONF_UNSET;
    
    ancf->begin_tm            = NGX_CONF_UNSET_UINT;
    ancf->anti_acqu_hash      = NULL;
    ancf->anti_shm_zone       = NULL;
    ancf->anti_frozen_hash    = NULL;

    return ancf;
}


static char * 
ngx_anti_frozen_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_anti_conf_t *ancf = conf;
    ngx_str_t  *value;

    if (ancf->anti_frozen_hash_size != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    ancf->anti_frozen_hash_size = ngx_atoi(value[1].data, value[1].len);
    if (ancf->anti_frozen_hash_size == NGX_ERROR) {
        return "invalid number";
    }

    // hashtable的数组长度不能超过1M， 分页大小是4096, 优化的时候可以把pagesize的大小调大
    if (ancf->anti_frozen_hash_size <= 0 || ancf->anti_frozen_hash_size > 1048576) {
        // ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get client addr error");
        return NGX_CONF_ERROR;
    }   

    // 分配内存，挂载ancf中
    ancf->anti_frozen_hash = ngx_pcalloc(cf->pool, sizeof(anti_hash_t *) * ancf->anti_frozen_hash_size);
    if (ancf->anti_frozen_hash == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char * 
ngx_anti_acqu_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_anti_conf_t *ancf = conf;
    ngx_str_t  *value;

    if (ancf->anti_acqu_hash_size != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    ancf->anti_acqu_hash_size = ngx_atoi(value[1].data, value[1].len);
    if (ancf->anti_acqu_hash_size == NGX_ERROR) {
        return "invalid number";
    }

    if ( ancf->anti_acqu_hash_size <= 0 || ancf->anti_acqu_hash_size > 1048576) {
        return NGX_CONF_ERROR;
    }
    
    ancf->anti_acqu_hash = ngx_pcalloc(cf->pool, sizeof(anti_hash_t *) * ancf->anti_acqu_hash_size );
    if (ancf->anti_acqu_hash == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;

}

static ngx_int_t 
anti_shm_init(ngx_shm_zone_t *zone, void *data) {
    return NGX_OK;
}


static char * 
ngx_anti_shm_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_anti_conf_t *ancf = conf;
    ngx_str_t  *value;
    // ngx_int_t  *np;

    if (ancf->anti_shm_size != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    ancf->anti_shm_size = ngx_atoi(value[1].data, value[1].len);
    if (ancf->anti_shm_size == NGX_ERROR) {
        return "invalid number";
    }

     // 开启共享内存分配内存, 共享内存大小不超过64M
    if (ancf->anti_shm_size <= 0 || ancf->anti_shm_size > 67108864) {
        return NGX_CONF_ERROR;
    }

    ngx_shm_zone_t   *shm_zone;
    ngx_str_t name = ngx_string("anti_hash");

    ngx_str_t *p = ngx_pcalloc(cf->pool, sizeof(ngx_str_t) + sizeof(u_char*) * (name.len));
    p->len       = name.len;
    p->data      = (u_char*)p + sizeof(ngx_str_t);
    ngx_memcpy(p->data, name.data, name.len);

    shm_zone = ngx_shared_memory_add(cf, p, ancf->anti_shm_size,
                                     &ngx_http_anti_module);

    ancf->anti_shm_zone = shm_zone;
    shm_zone->init       = anti_shm_init;

    return NGX_CONF_OK;
}



static char * 
ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_anti_conf_t *prev = parent;
    ngx_http_anti_conf_t *conf = child;

    ngx_conf_merge_value(conf->anti_open, prev->anti_open, 0);
    ngx_conf_merge_value(conf->anti_shm_size, prev->anti_shm_size, 8096);
    ngx_conf_merge_value(conf->anti_acqu_cycle, prev->anti_acqu_cycle, 60);
    ngx_conf_merge_value(conf->anti_acqu_type, prev->anti_acqu_type, 1);
    ngx_conf_merge_value(conf->anti_threshold, prev->anti_threshold, 10000);
    ngx_conf_merge_value(conf->anti_frozen_innernet, prev->anti_frozen_innernet, 1);
    ngx_conf_merge_value(conf->anti_frozen_time, prev->anti_frozen_time, 60);
    ngx_conf_merge_value(conf->anti_acqu_hash_size, prev->anti_acqu_hash_size, 128);
    ngx_conf_merge_value(conf->anti_frozen_hash_size, prev->anti_frozen_hash_size, 128);

    return NGX_CONF_OK;
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