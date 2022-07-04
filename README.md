# C++ webserver
C++ 简单webserver

基于异步IO Epoll与线程池实现的半同步/半反应堆模式的简单WEB服务器

状态机模型实现的服务器解析（仅解析GET命令）

利用时间堆来设定每个连接的最大允许连接时间
