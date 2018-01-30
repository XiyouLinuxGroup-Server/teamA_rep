/*************************************************************************
	> File Name: threadpool_test.cpp
	> Author: 吕白
	> Purpose:
	> Created Time: 2018年01月30日 星期二 15时49分26秒
 ************************************************************************/

#include<pthread.h>
#include<iostream>
#include<stdlib.h>
#include<unistd.h>
#include<cstdio>
#include<string>
#include<vector>
#include<queue>

using namespace std;

//执行任务的类：设置任务数据并执行
class CTask 
{
protected:
    string m_strTasKNanme;   //任务名称
    void *m_ptrData;        //要执行的任务的具体数据

public:
    CTask() = default;
    CTask( string &taskName ) : m_strTasKNanme( taskName ), m_ptrData( NULL ){}
    virtual int Run() = 0;
    void setData( void *data );     //设置任务数据并执行

    virtual ~CTask() {}
};

//线程池管理类
class CThreadPool 
{
private:
    static vector< CTask* > m_vecTasKList; //任务列表
    static bool shutdown; //线程退出标志
    int m_iThreadNum; //线程中启动的线程数
    pthread_t *pthread_id;

    static pthread_mutex_t m_pthreadMutex;  //线程同步锁
    static pthread_cond_t m_pthreadCond;   //线程同步变量
    
protected:
    static void *ThreadFunc( void *threadData );     //新线程的线程回调函数
    static int MoveToIdle( pthread_t tid );     //线程执行结束后，把自己放入空闲线程中
    static int MoveToBusy( pthread_t tid );     //移入到忙碌线程中去
    int Create();   //创建线程池中的线程

public:
    CThreadPool( int threadNum );
    int AddTask( CTask *task );     //把任务添加到任务队列
    int StopAll();  //使线程池中的所有线程退出
    int getTaskSize();  //获取当前任务队列中的任务数

};

void CTask :: setData( void *data ) 
{
    m_ptrData = data;
}

//静态成员初始化
vector< CTask* > CThreadPool :: m_vecTasKList;
bool CThreadPool :: shutdown = false;
pthread_mutex_t CThreadPool :: m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool :: m_pthreadCond = PTHREAD_COND_INITIALIZER;

//线程管理函数
CThreadPool :: CThreadPool( int threadNum ) 
{
    this->m_iThreadNum = threadNum;
    cout << " i will create " << threadNum << " threads\n";
    Create();
}

//线程回调函数
void *CThreadPool :: ThreadFunc( void *threadData ) 
{
    pthread_t tid = pthread_self();
    while (1)
    {
        pthread_mutex_lock( &m_pthreadMutex );
        //如果队列为空，等待型任务进入任务队列
        while ( m_vecTasKList.size() == 0 && !shutdown )
        {
            pthread_cond_wait( &m_pthreadCond, &m_pthreadMutex );
        }

        //关闭线程
        if ( shutdown )
        {
            pthread_mutex_unlock( &m_pthreadMutex );
            printf( "[tid: %lu]\texit\n", pthread_self() );
            pthread_exit( nullptr );
        }

        printf("[tid: %lu]\trun: ", tid);
        vector<CTask*> :: iterator iter = m_vecTasKList.begin();
        //取出一个任务并处理之
        CTask *task = *iter;
        if ( iter != m_vecTasKList.end() )
        {
            task = *iter;
            m_vecTasKList.erase( iter );
        }
        pthread_mutex_unlock( &m_pthreadMutex );

        task->Run();    //执行任务
        printf( "[tid: %lu]\tidle\n", tid );
    }
    return ( void * ) 0;
}

//往任务队列里添加任务并发出线程同步信号
int CThreadPool :: AddTask( CTask *task ) 
{
    pthread_mutex_lock( &m_pthreadMutex );
    m_vecTasKList.push_back( task );
    pthread_mutex_unlock( &m_pthreadMutex );
    pthread_cond_signal( &m_pthreadCond );

    return 0;
}

//创建线程 
int CThreadPool :: Create() 
{
    pthread_id = new pthread_t[ m_iThreadNum ];
    for( int i =0 ; i < m_iThreadNum; i++)
    {
        pthread_create( &pthread_id[i], nullptr, ThreadFunc, nullptr );
    }

    return 0;
}

//停止所有线程
int CThreadPool :: StopAll() 
{
    //避免重复调用
    if ( shutdown )
    {
        return -1;
    }
    printf( "Now i will end all threads!\n" );

    //唤醒所有等待线程，线程池也要销毁了
    shutdown = true;
    pthread_cond_broadcast( &m_pthreadCond );

    //清除僵尸
    for( int i = 0; i < m_iThreadNum; i++ )
    {
        pthread_join( pthread_id[i], nullptr );
    }
    delete[] pthread_id;
    pthread_id = nullptr;

    //销毁互斥量和条件变量
    pthread_mutex_destroy( &m_pthreadMutex );
    pthread_cond_destroy( &m_pthreadCond );

    return 0;
}

//获取当前队列中的任务数
int CThreadPool :: getTaskSize() 
{
    return m_vecTasKList.size();
}

class CMyTask : public CTask 
{
public:
    CMyTask() = default;
    int Run()
    {
        printf( "%s\n", (char*)m_ptrData );
        int x = rand()%4 + 1;
        sleep( x );
        return 0;
    }
    ~CMyTask() {}
};

int main()
{
    CMyTask taskObj;
    char szTmp[] = "hello!";
    taskObj.setData( (void*)szTmp );
    CThreadPool threadpool( 5 );

    for( int i = 0; i < 10; i++ )
    {
        threadpool.AddTask( &taskObj );
    }
    while( 1 )
    {
        printf( "There are still %d tasks need to handle\n", threadpool.getTaskSize() );
        //任务队列已经没有任务了
        if ( threadpool.getTaskSize() == 0 )
        {
            //清除线程
            if ( threadpool.StopAll() == -1 )
            {
               printf( "Threadpool clear,exit\n" );
                exit(0);
            }
        }

        sleep(2);
        printf( "2 seconds later...\n" );
    }
    return 0;
}
