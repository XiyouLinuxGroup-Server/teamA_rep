## 说明
通过对线程池和epoll的学习，此次小项目结合了线程池和epoll编写了一个实现了time,echo,discard,daytime,chargen功能的粗糙的服务器。。。

## 编译说明
服务器：
```
g++ myepoll.cpp -o myepoll -std=c++11 -lpthread
```
客户端：
```
g++ client.cpp -o client -std=c++11 -lpthread  
```
## 运行
服务器：
```
./myepoll
```
客户端：
```
./client
```

