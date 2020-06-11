# hctunnel



### 介绍



hctunnel 是一个http tunnel工具，能够同时处理http和https请求。原理为，客户端首先与代理服务器建立TCP连接，然后代理服务器将请求转发给目标服务器，并且将目标服务器返回的内容转发给客户端。对于http请求，会直接将请求转发，对于https请求，代理服务器会首先回应客户端的CONNECT请求，然后转发HTTPS数据。





### 使用



```bash
cmake .
make
./hctunnel
```



自己在腾讯云上部署了一个最新版本的hctunnel，可以设置HTTP代理为`106.55.4.241:8888`来使用。例如，



```
user@ubuntu:~/hctunnel$ curl -x http://106.55.4.241:8888 myip.ipip.net
当前 IP：106.55.4.241  来自于：中国 广东 广州  电信/联通/移动
```

