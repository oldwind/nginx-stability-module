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




