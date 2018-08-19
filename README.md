服务稳定性模块

稳定性模块主要是在webservice中，提供页面的缓存、降级能力，面向的业务有两个部分
 > a、大部分一级功能和更新频率不是特别高的页面，例如，首页、榜单、资讯详情页等
 > b、针对缓存信息比较大的例如资讯列表，采用LRU CACHE 方式收集访问热点做缓存

同时, 支持在URL维度设定缓存时间

1、模块实现的功能
    实现缓存


2、编译安装
    nginx的release版本下加入本模块

    ./configure --prefix=/opt/demo/nginx --add-module=/Users/baidu/github/nginx-stability-module  --with-http_stub_status_module 

 > --prifix 安转目录
 > --add-module 加入第三方模块地址
 > --with-http_stub_status_module 加入非默认的第三方库地址


3、nginx.conf配置说明


4、实现指令的说明

