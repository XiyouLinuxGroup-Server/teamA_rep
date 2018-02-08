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
#include<fcntl.h>
#include<errno.h>
#include<cstdlib>
#include<string>
#include<vector>
#include<queue>
#include<list>
using namespace std;

const int MAxBuf = 100;
const int MinUsual = 5;
const int MaxUsual = 10;

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


//线程池的接口　管理线程池
class ThreadManage 
{
    
private:
    ThreadPool *pool;   //指向实际的线程池
    int Numthread;  //默认创建的线程数
public:
    ThreadManage();
    ~ThreadManage();
};

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
    pthread_cond_signal( &cond );
    pthread_mutex_unlock( &mutex );

}

void ThreadPool :: StopAll() 
{
    if ( shutdown ) 
    {
        return ;
    }
    shutdown = true;
    pthread_cond_broadcast( &cond );
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
        JobList.erase( itr );
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

class MyJob :: public Job 
{
public:
    void Run()
    {

    }
}

class MyEpoll 
{
    
}
