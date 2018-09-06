### 服务稳定性模块

#### 1、模块实现的功能
- 页面主要提供location的防刷功能，通过指令配置，针对请求超过阈值的IP + URL，返回403， 进行请求冻结

- @todo list
    - 1、多维度防刷 支持IP防刷，URL防刷，IP+URL综合防刷
    - 2、优化hashtable存储结构

#### 2、编译安装
    nginx的release版本下加入本模块

    ./configure --prefix=/Users/baidu/dev/nginx --add-module=/{$USER_PATH}/github/nginx-stability-module/

- --prifix 安转目录
- --add-module 加入第三方模块地址
- --with-http_stub_status_module 加入非默认的第三方库地址

#### 3、nginx.conf配置说明

##### 1. anti_shm_size : [1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : on <br/> 
> 说明 : 申请数据采集和冻结功能的共享内存大小 <br/> 
   
##### 2. anti_acqu_cycle : [ 1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : on <br/> 
> 说明 : 数据采集的周期，单位是秒 <br/> 

##### 3. anti_threshold : [ 1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : 10000 <br/> 
> 说明 : 启动冻结功能的阈值，单位是秒 <br/> 
   
##### 4. anti_frozen_time : [ 1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : 60 <br/> 
> 说明 : 启动冻结的时长 <br/> 

##### 5. anti_acqu_hash_size : [ 1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : 128 <br/>  
> 说明 : 数据采集存储的hashtable 数组的大小，并发越高，该值建议设大 <br/> 

##### 6. anti_frozen_hash_size : [ 1 .. 102400000000]  <br/> 
> context : location <br/> 
> default : 128 <br/> 
> 说明 : 服务冻结存储的hashtable 数组的大小，冻结数据越多，该值建议设大 <br/> 


