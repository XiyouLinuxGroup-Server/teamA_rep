/*************************************************************************
	> File Name: myepoll.cpp
	> Author: hujialu
	> Mail: 
	> Created Time: 2018年02月28日 星期三 23时18分21秒
 ************************************************************************/
#include"threadpools.h"
#include<iostream>
using namespace std;
const char *ip = "127.0.0.1";
class myepoll
{
public:
    myepoll();
    ~myepoll();
    void init();
    int wait();
    void epoll_recv(int sockfd);//接受
    void epoll_send(int sockfd);//发送
    void epoll_close(int sockfd);//关闭
    void add_client();//增加新的客户端
    void my_epoll();//epoll框架
private:
    int sockfd;
    int epfd;
    threadpools *pool;
    job myjob;
    struct epoll_event ev, *event;
};

myepoll::myepoll()
{
    sockfd = -1;
    epfd = -1;
    printf("9993\n");
    event = new epoll_event[20];
    printf("9993\n");
    pool = new threadpools;
    printf("9993\n");
}

myepoll::~myepoll()
{
    delete[] event;
    delete pool;
    pool = NULL;
    event = NULL;
}
/*创建套接字，绑定并且监听*/
void myepoll :: init()
{
    int ret;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(4507);

    sockfd = socket(PF_INET, SOCK_STREAM,0);
    assert(sockfd != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    epfd = epoll_create(20);//创建内核事件表描述符

    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET; //ET模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);//注册新事件
}
void myepoll::epoll_recv(int sockfd)
{
    struct tmp{
        int s;
        int n;
        char buf[256];
    };
    struct tmp buf;
    int bytes;
    if(bytes = recv(sockfd, (void *)&buf, sizeof(buf), 0) <=0 )
    {
        cout << "error\n";
    }
    else{
        pool->add_joblist(&myjob);
    }
}

void myepoll::epoll_send(int sockfd)
{
    
}

void myepoll::epoll_close(int sockfd)
{
    close(sockfd);
}
void myepoll::add_client()//增加新连接
{
    sockaddr_in client_addr;
    memset(&client_addr, 0,sizeof(client_addr));
    socklen_t clien = sizeof(client_addr);
    int client = accept(sockfd,(struct sockaddr*)&client_addr, &clien);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
}

int myepoll::wait()
{
    int num;
    num = epoll_wait(epfd, event, 20, -1);
    return num;
}

void myepoll::my_epoll()
{
    int num;
    while(1)
    {
        num = wait();
        if(num == 0)
        {
            cout << "超时\n";
            continue;
        }
        if(num == -1)
        {
            cout << "error\n";
        }
        if(num > 0)
        {
            for(int i=0; i<num; i++)
            {
                if(event[i].data.fd == sockfd)
                {
                    add_client();
                }
                else{
                    //有可读就绪事件
                    if(event[i].events & EPOLLIN)
                    {
                        epoll_recv(event[i].data.fd);
                    }
                    else if(event[i].events & EPOLLOUT)
                    {
                        epoll_send(event[i].data.fd);
                    }
                }
            }
            memset(&event, 0, sizeof(event));
        }
    }
}

int main()
{
    printf("999\n");
    myepoll epoll;
    printf("999\n");
   //< epoll.init();
   // epoll.my_epoll();
    return 0;
}
