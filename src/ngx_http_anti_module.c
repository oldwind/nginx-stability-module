/*
 * Copyright (C) yebin
 */

// #ifndef _ngx_http_anti_
// #define _ngx_http_anti_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

// 计数器的hash结构，开链方式，val统一是int类型
typedef struct anti_acqu_hash_t{    
    ngx_str_t acqu_key;
    ngx_int_t acqu_val;
    struct anti_acqu_hash_t * next;
} anti_acqu_hash_t;


// 冻结数据记录信息
typedef struct anti_frozen_hash_t {
    ngx_int_t expire_tm;
    anti_acqu_hash_t acqu_hash;
} anti_frozen_hash_t;

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

    // queue，支持数据采集的周期形成，后期实现
    
    ngx_int_t  begin_tm;  // 采集周期开始时间

    anti_acqu_hash_t     **anti_acqu_hash;
    ngx_shm_zone_t  *anti_shm_zone;  // 共享内存，初始化为NULL
 
    anti_frozen_hash_t **anti_frozen_hash;  // 冻结地址信息记录

} ngx_http_anti_conf_t;


static ngx_int_t 
anti_hash_tbl_del(ngx_http_anti_conf_t *ancf, void * dp); 

static ngx_int_t 
anti_shm_init(ngx_shm_zone_t *zone, void *data);

static char * ngx_anti_shm_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_anti_acqu_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_anti_frozen_hash_size_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t  ngx_http_anti_postconf_init(ngx_conf_t *cf);
static void *  ngx_http_anti_create_loc_conf(ngx_conf_t *cf);
static char *  ngx_http_anti_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_anti_handler(ngx_http_request_t *r);

static anti_acqu_hash_t * 
    anti_hash_tbl_find(anti_acqu_hash_t  ** anti_hash, ngx_str_t * str, ngx_int_t hash_size);

static ngx_int_t 
    anti_hash_tbl_set(ngx_http_anti_conf_t *ancf,  ngx_str_t * str,  ngx_int_t val); 


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


// 1、判断配置的监控类别 (IP, IP + URL， IP +args参数等)
// 2、根据类别组装查询的KEY
// 3、从冻结的hashtable里面查询KEY是否存在，@todo 同时清理
//    3.1 如果存在，并且没有过期，返回请求forbidden
//    3.2 如果不存在，或者存在的但是过期， 继续4
// 4、加共享锁
// 5、判断数据采集的hashtable是否到周期
//    5.1 到周期，循环清理内存
//    5.2 未到周期，继续6
// 6、判断KEY在数据采集hashtable中是否存在
//    6.1 存在， +1, 判断是否超过阈值
//        6.1.1 超过阈值，查询冻结表是否存在，存在则更新解冻时间，返回forbidden
//    6.2 不存在， 增加KEY，初始化1
// 7、关闭共享锁 (4-7之间异常情况关闭贡献锁)
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
  
    
    // sprintf(str, "ip=%d", (int)s_addr);   
    // ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, str);

    if (len <= 0) {
        return NGX_OK;
    }

    ngx_time_t *now;
    ngx_timezone_update();
    now = ngx_timeofday();
    ngx_time_update();


    // 5.1 判断采集开始周期，超过周期，清空内存
    anti_acqu_hash_t  *tmp1, *tmp2;
    if (ancf->anti_acqu_cycle - ancf->begin_tm > now->sec) {
        for (int i = 0; i < ancf->anti_acqu_hash_size; i ++) {
            if (ancf->anti_acqu_hash[i] == NULL) {
                continue;
            }
            ancf->anti_acqu_hash[i] = NULL;
            tmp1 = ancf->anti_acqu_hash[i]->next;
            while (tmp1 != NULL) {
                tmp2 = tmp1;
                tmp1 = tmp1->next;
                anti_hash_tbl_del(ancf, tmp2);
            }
        }
    }

    // 6. 查找是否在收集的hashtable中  // 测试hashtable find
    ngx_str_t test_str = ngx_string("/sisisisis?sdajfda");

    anti_acqu_hash_t *hash_temp = anti_hash_tbl_find(ancf->anti_acqu_hash, &test_str, ancf->anti_acqu_hash_size);
   
    ngx_int_t num = hash_temp->acqu_val;
    if (hash_temp != NULL) {
        // anti_hash_tbl_set(ancf, &test_str, hash_temp->acqu_val + 1);
        hash_temp->acqu_val = hash_temp->acqu_val + 1; // 加锁保证原子性
        num = hash_temp->acqu_val + 1;
    } else {
        anti_hash_tbl_set(ancf, &test_str, 1);
        num = 1;
    }


    char str[100];
    sprintf(str, "request_tm=%ld", num);
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, str);


    // 6.1.1 判断是否超过阈值, 写冻结的hashtable中
    if ( hash_temp->acqu_val > ancf->anti_threshold) {

    }



    // 写到共享内存

    

    return NGX_OK;
}


// 从cache里面查到string是否存在, 查的过程中，发现如果链表数据过期，将数据删除
// 支持 anti_acqu_hash_t 类型和 包含该类型的hashtable的链表遍历查找，例如 anti_frozen_hash_t
// hash算法选择mermerhash算法
static anti_acqu_hash_t * 
anti_hash_tbl_find(anti_acqu_hash_t ** header, ngx_str_t * find_str, ngx_int_t hash_size) {
    if (header == NULL) {
        return NULL;
    }

    uint32_t hash_base_size = ngx_murmur_hash2(find_str->data, find_str->len);
   
    anti_acqu_hash_t *hash_temp;
    
    hash_temp = header[hash_base_size % hash_size];
    
    while (hash_temp != NULL ) {

        if (0 == ngx_strcmp(hash_temp->acqu_key.data, find_str->data)) {
            return hash_temp;
        }

        hash_temp = hash_temp->next;
    }

    return NULL;
} 

// 将str值放在hash表的最后位置
static ngx_int_t 
anti_hash_tbl_set(ngx_http_anti_conf_t *ancf,  ngx_str_t * str,  ngx_int_t val) {

    ngx_slab_pool_t  *shpool;
    anti_acqu_hash_t *acqu_save;
    anti_acqu_hash_t **anti_acqu_hash = ancf->anti_acqu_hash;
    anti_acqu_hash_t *hash_temp;
    
    // ngx_int_t anti_acqu_hash_size  = ancf->anti_acqu_hash_size;
    uint32_t hash_base_size = ngx_murmur_hash2(str->data, str->len);

    // 未查到str信息，那么新建一个str信息，先检查在共享内存中申请的大小
    // 申请完毕后，将str值放到响应结构体中
    ngx_int_t size = sizeof(anti_acqu_hash_t) + str->len;
    
    // 共享内存池
    shpool = (ngx_slab_pool_t *) ancf->anti_shm_zone->shm.addr;


    acqu_save = ngx_slab_alloc(shpool, size);
    if (acqu_save == NULL) {
        return NGX_ERROR;
    }

    acqu_save->next          = NULL;
    acqu_save->acqu_val      = val;
    acqu_save->acqu_key.len  = str->len;
    acqu_save->acqu_key.data = (u_char * ) (acqu_save + sizeof(anti_acqu_hash_t));

    ngx_memcpy(acqu_save->acqu_key.data, str->data, str->len);

    hash_temp = anti_acqu_hash[hash_base_size % ancf->anti_acqu_hash_size];
    
    if (hash_temp == NULL) {
        anti_acqu_hash[hash_base_size % ancf->anti_acqu_hash_size] = acqu_save;
    } else {
        while (hash_temp->next != NULL) {
            hash_temp = hash_temp->next;
        }
        hash_temp->next = acqu_save;
    }
    
    return NGX_OK;
} 


static ngx_int_t 
anti_hash_tbl_del(ngx_http_anti_conf_t *ancf, void * dp) 
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
    ancf->anti_frozen_hash = ngx_pcalloc(cf->pool, sizeof(anti_frozen_hash_t *) * ancf->anti_frozen_hash_size);
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
    
    ancf->anti_acqu_hash = ngx_pcalloc(cf->pool, sizeof(anti_acqu_hash_t *) * ancf->anti_acqu_hash_size );
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
    ngx_conf_merge_value(conf->anti_acqu_cycle, prev->anti_acqu_cycle, 1);
    ngx_conf_merge_value(conf->anti_acqu_type, prev->anti_acqu_type, 1);
    ngx_conf_merge_value(conf->anti_threshold, prev->anti_threshold, 100);
    ngx_conf_merge_value(conf->anti_frozen_innernet, prev->anti_frozen_innernet, 1);
    ngx_conf_merge_value(conf->anti_frozen_time, prev->anti_frozen_time, 300);
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