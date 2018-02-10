/*************************************************************************
	> File Name: sever.cpp
	> Author: 吕白
	> Purpose:
	> Created Time: 2018年02月05日 星期一 16时47分37秒
 ************************************************************************/

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<signal.h>
#include<iostream>
#include<unistd.h>
#include<assert.h>
#include<fcntl.h>
#include<errno.h>
#include<cstdlib>
#include<cstring>
#include<string>
#include<vector>
#include<queue>
#include<list>
using namespace std;

const int MAxBuf = 100;
const int MinUsual = 5;
const int MaxUsual = 10;
const int PORT = 4507;
const char *ip = "127.0.0.1";
const int sock_count = 256;

//任务的基类
class Job 
{
protected:
    int Number;     //任务选项
    char *buf;      //缓冲区
public:
    Job();
    ~Job();
    virtual void Run() = 0;
    
};

//实际的线程池
class ThreadPool 
{

protected:
    static void *FunThread( void *arg );    //线程的调度函数
private:
    static bool shutdown;   //标志着线程的退出
    static vector <Job *> JobList;  //任务列表
    static pthread_mutex_t mutex;
    static pthread_cond_t cond;

    pthread_t *pid;
    int InitNum;    //初始化线程数
    int NowNum;     //现在拥有的线程数
    int UsualMin;   //应该保持的最少的线程数
    int UsualMax;   //应该保持的最大空闲线程数
public:
    ThreadPool();
    ~ThreadPool();
    void create();   //创建线程池中的线程
    void StopAll();  //销毁线程池
    void AddJob( Job *job );   //添加任务
    void IncThread();   //向线程池中添加线程
    void RedThread();   //减少线程池中的线程
    int Getsize();  //获取当前任务队列中的任务
};

/*
//线程池的接口　管理线程池
class ThreadManage 
{
    
private:
    ThreadPool *pool;   //指向实际的线程池
public:
    ThreadManage();
    ~ThreadManage();
};
*/
Job :: Job()
{
    Number = -1;
    buf = new char[ MAxBuf ];
}

Job :: ~Job()
{
    Number = -1;
    delete[] buf;
    buf = nullptr;
}

/*
//线程池的接口
ThreadManage :: ThreadManage() 
{
    pool = new ThreadPool;
}

ThreadManage :: ~ThreadManage() 
{
    delete pool;
    pool = nullptr;
}
*/

//初始化静态变量
bool ThreadPool :: shutdown = false;
vector <Job *> ThreadPool :: JobList;
pthread_mutex_t ThreadPool :: mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool :: cond = PTHREAD_COND_INITIALIZER;

ThreadPool :: ThreadPool()
{
    InitNum = 5;
    NowNum = InitNum;
    UsualMax = MaxUsual;
    UsualMin = MinUsual;
    pid = new pthread_t[ InitNum ];
    create();
}

void ThreadPool :: create()
{
    for ( int i = 0; i < InitNum; i++ ) 
    {
        pthread_create( &pid[i], nullptr, FunThread, nullptr );
    }
}

void ThreadPool :: AddJob( Job *job ) 
{
    pthread_mutex_lock( &mutex );
    JobList.push_back( job );
    pthread_cond_signal( &cond );   //任务添加后发出信号
    pthread_mutex_unlock( &mutex );

}

void ThreadPool :: StopAll() 
{
    if ( shutdown ) //避免重复调用
    {
        return ;
    }
    shutdown = true;
    pthread_cond_broadcast( &cond );    //唤醒所有线程
    for ( int i = 0; i < NowNum; i++ ) 
    {
        pthread_join( pid[i], nullptr );
    }
    delete[] pid;
    pid = nullptr;
}

void* ThreadPool :: FunThread( void *arg ) 
{
    pthread_t tid = pthread_self();
    while (1) 
    {
        pthread_mutex_lock( &mutex );
        //如果没有任务且仍在正常运行　等待
        while ( !shutdown && JobList.size() == 0 ) 
        {
            pthread_cond_wait( &cond, &mutex );     //等待添加任务的唤醒
        }

        //退出线程
        if ( shutdown ) 
        {
            pthread_mutex_unlock( &mutex );
            cout << tid << " exit\n";
            pthread_exit( nullptr );
        }
        
        //取出一个任务
        vector <Job *> :: iterator itr = JobList.begin();
        Job *job = *itr;
        JobList.erase( itr ); //从任务队列删除取出的任务
        pthread_mutex_unlock( &mutex );
        job->Run();
    }
}

void ThreadPool :: IncThread() 
{

}

void ThreadPool :: RedThread() 
{
    
}

int ThreadPool :: Getsize()  
{
    return JobList.size();
}

class MyJob : public Job    //实际的任务类 
{
public:
    void Run();
};

void MyJob :: Run()     //执行任务
{
    
}


//封装epoll类
class MyEpoll 
{
public:
    MyEpoll();
    ~MyEpoll();
    void Init();    //初始化
    int Epoll_wait();   //等待
    void Epoll_new_client();     //新客户端
    void Epoll_recv( int sockfd );   //接收数据
    void Epoll_send( int sockfd );   //发送数据
    void parsing( MyJob &task, char *buf );     //分析数据执行哪个协议
    void Epoll_close();     //断开连接
    void Epoll_run();   //运行
private:
    int sock;
    int epfd;
    ThreadPool *pool;
    struct epoll_event ev, *event;  //ev用于处理事件　event数组用于回传要处理的事件
};

MyEpoll :: MyEpoll() 
{
    sock = 0;
    epfd = 0;
    event = new epoll_event[20];
    pool = new ThreadPool;
}

MyEpoll :: ~MyEpoll() 
{
    delete[] event;
    delete pool;
    pool = nullptr;
    event = nullptr;
}

void MyEpoll :: Init() 
{
    int ret;
    struct sockaddr_in address;
    bzero( &address, sizeof(address) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    //address.sin_addr.s_addr = htonl( INADDR_ANY );
    address.sin_port = htons( PORT );
   
    sock = socket( PF_INET, SOCK_STREAM, 0 );   
    assert( sock != -1 );
    
    ret = bind( sock, (struct sockaddr*)&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, 5 );
    assert( ret != -1 );

    epfd = epoll_create( 1 );
    //epfd = epoll_create( sock_count );  //创建epoll句柄
    
    ev.data.fd = sock;  //设置与要处理的事件相关的文件描述符
    ev.events = EPOLLIN | EPOLLET;  //设置为ET模式
    epoll_ctl( epfd, EPOLL_CTL_ADD, sock, &ev );    //注册新事件
}

int MyEpoll :: Epoll_wait()     //等待事件 
{
    return epoll_wait( epfd, event, 20, 500 );
}

void MyEpoll :: Epoll_new_client() 
{
    sockaddr_in client_addr;
    bzero( &client_addr, sizeof(client_addr) );
    socklen_t clilen = sizeof( client_addr );
    
    int client = accept( sock, (struct sockaddr*)&client_addr, &clilen ); //接受连接
    
    ev.events = EPOLLIN | EPOLLET;  
    ev.data.fd = client;    
    epoll_ctl( epfd, EPOLL_CTL_ADD, client, &ev );  //注册新事件

}

void MyEpoll :: Epoll_recv( int sockfd )
{
    MyJob task;
    char buf[256];
    int nbytes;
    if ( nbytes = recv( sockfd, buf, sizeof(buf), MSG_WAITALL ) <= 0 )  //阻塞模式接收
    {
        ev.data.fd = sockfd;
        ev.events = EPOLLERR;   //对应文件描述符发生错误
        epoll_ctl( epfd, EPOLL_CTL_DEL, sockfd, &ev );
        close( sockfd );
        return ;
    }
    parsing( task, buf ); //分析数据执行哪个协议
    pool->AddJob( &task ); 
}

void MyEpoll :: parsing( MyJob &task, char *buf ) 
{
    :wq
}

void MyEpoll :: Epoll_send( int sockfd ) 
{
    
}
