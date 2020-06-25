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
服务器上hctunnel的运行状态如下：

![](./img/runningstate.png)

hctunnel的pid为1'1415，已经运行了11天11小时28分钟，cpu时间为25小时25分钟，目前占用内存3628KB，大约3.5兆。共处理了17'7511次代理请求，读入字节数约640兆，写出字节数约810兆。



### 实现

hctunnel是基于reactor事件驱动模型，只使用c++标准库完成的。默认开启五个线程，每个线程都有自己的事件循环。主线程用来accept新连接，并且分发连接给四个worker线程。worker线程用来管理连接，处理IO。

hctunnel的**内部实现**从源文件来看，主要有这几个class：

- Server：主线程中运行，有一个全局可访问的单例。负责accept新连接，并将新连接分配给线程池其中的一个线程。
- EventLoop：使用epoll作为多路复用机制。loop中监听事件，并且分发事件。由于EventLoop会被主线程和自己的线程两个线程调用，因此需要互斥量mutex。条件变量cv用来休眠和唤醒loop。
- IOWatcher：借鉴自libev中watcher的概念。IOWatcher作为一个抽象基类，提供了文件IO所必要的变量
- Postman：继承自IOWatcher，用来实现http tunnel。Postman是一个孪生的结构，分为local postman和remote  postman。local postman代表客户端与hctunnel这条连接，remote postman代表hctunnel与服务端的连接。根据这两种类型的postman，数据有两种流向：upstream、downstream，具体被分为四部分：upstreamRead、upstreamWrite、downstreamRead、downstreaWrite。

![](./img/datastream.png)



这几个class与reactor几个角色之间的**对应关系**：

| reactor角色                     | hctunnel中的class |
| ------------------------------- | ----------------- |
| Synchronous Event Demultiplexer | EventLoop         |
| Initiation Dispatcher           | EventLoop         |
| Event Handler                   | IOWatcher         |
| Concrete Event Handler          | Postman           |



**线程安全的考虑**：

来自客户端的每条socket连接都分为两个阶段：accept阶段和IO阶段。其中accept阶段是由主线程处理，也就是Server来处理。IO阶段由其所归属的worker线程处理，连接关闭也发生在worker线程。将两个阶段独立来看都是线程安全的，不需要任何的同步措施。

但是，在accept阶段和IO阶段之间，也就是主线程accept之后，worker线程监听连接上的事件之前，Server需要将这个连接交给某个EventLoop。此时需要考虑线程安全，因为EventLoop会同时被主线程和自己所属的线程调用。保证线程安全，需要找出临界区和互斥资源。临界区很好找，因为Server将连接交给EventLoop只需要调用EventLoop的addFd成员函数。因此，临界区就是addFd成员函数，互斥资源就是addFd函数内所涉及的成员变量。

同时，EventLoop下所有对互斥资源的访问都要加锁。

![](./img/threadsafecode.png)





### 待实现

- 使用eventfd替代条件变量实现线程的异步唤醒
- 使用time heap的方式代替目前的超时机制和epoll_wait的timeout参数
- 当域名无效以及其他http错误时，返回错误信息
- 单独开一个线程，用来记录日志
- 非阻塞connect
- "tcp segment of a reassembled pdu" 问题。（python的requests包，使用proxies选项）