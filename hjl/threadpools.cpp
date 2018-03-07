/*************************************************************************
	> File Name: server1.cpp
	> Author: hujialu
	> Mail: 
	> Created Time: 2018年02月28日 星期三 20时42分46秒
 ************************************************************************/

#include<sys/socket.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<vector>
#include<queue>
#include<list>
#include<string>
#include<string.h>
#include<stdlib.h>
#include<iostream>
using namespace std;

typedef struct pthreadpool
{
    pthread_t id;
    struct pthreadpool *next;
}thpid;

//任务基本类型
class job
{
protected:
    int sockfd;//套接字
    int number;//功能选择
    char *buf;//缓冲区
public:
    job();
    ~job();
    int get_sockfd();
    int get_number();
    char *get_buf();
    void Daytime(int sockfd, char *buf);
    void Time(int sockfd, char *buf);
    void echo(int sockfd, char *buf);
    char *discard(int sockfd, char *buf);
    void chargen(int sockfd, char *buf);
    void job_run(int sockfd,int number,char *buf);
};

job::job()
{
    sockfd = -1;
    number = -1;
    memset(buf, 0, 256);
}

job::~job()
{
    
}

int job::get_sockfd()
{
    return sockfd;
}

int job::get_number()
{
    return number;
}

char *job::get_buf()
{
    return buf;
}

void job::echo(int sockfd, char *buf)
{
    send(sockfd, (void *)buf, 256, 0);
}

void job::Daytime(int sockfd, char *buf)
{
    memset(&buf, 0, 256);
    time_t t;
    tm *local;
    t = time(NULL);
    local = localtime(&t);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S\n",local);
    send(sockfd, (void *)buf, 256, 0);
}

void job::Time(int sockfd, char *buf)
{
    time_t t;
    t = time(NULL);
    char tmp[256];
    sprintf(tmp, "%d",(int)t);
    int change;
    int k = 0;
    int mask = 256;
    char bit;
    for(short i=0; i< strlen(tmp); i++)
    {
        for(int j=0; j<9; j++)
        {
            buf[k++] = (mask & (tmp[i]))? 49 : 48;
            mask >>=1;
        }
        mask = 256;
    }
    send(sockfd, (void *)buf, 256, 0);
}

void job::chargen(int sockfd, char *buf)
{
    int ran;
    char str[63]="abcdefghijklmnopqrstuvwxyzQWERTYUIOPASDFGHJKLZXCVBNM0123456789";
    for(int i=0; i<10; i++)
    {
        ran = rand()%62;
        *buf = str[ran];
        buf++;
    }
    *buf = '\0';
    send(sockfd, (void *)buf, 256, 0);
}

char* job::discard(int sockfd,char *buf)
{
    memset(buf, 0, 256);
    send(sockfd, (void *)buf, 256, 0);
}
void job::job_run(int sockfd, int number, char *buf)
{
    switch(number)
    {
        case 1://echo
        {
            echo(sockfd,buf);
            break;
        }
        case 2://chargen
        {
            do{
                sleep(1);
                chargen(sockfd,buf);
            }while((send(sockfd, (void *)buf, 256,0))!=-1);
            break;
        }
        case 3://Time
        {
            Time(sockfd,buf);
            break;
        }
        case 4://Daytime
        {
            Daytime(sockfd,buf);
            break;
        }
        case 5://discard
        {
            discard(sockfd,buf);
            break;
        }
    }
}

class threadpools
{
protected:
    static void *fun(void *arg);//线程调用函数
private:
    static int flag;
    static pthread_mutex_t mutex;//锁
    static pthread_cond_t cond;//标志量
    static vector <job *> joblist;
    thpid *Pid;
public:
    threadpools();
    ~threadpools();
    void create_threads();//创建线程池
    void add_threads();//增加线程
    void dele_threads();//从线程中删除线程
    void add_joblist(job *myjob);//增加工作任务
};

int threadpools :: flag = 0;
vector <job *> threadpools :: joblist;
pthread_mutex_t threadpools :: mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threadpools :: cond = PTHREAD_COND_INITIALIZER;

threadpools::threadpools()
{
    Pid = (thpid *)malloc(sizeof(thpid));
    create_threads();

}
threadpools::~threadpools()
{
    free(Pid);
}

void threadpools :: create_threads()
{
    thpid *q;
    thpid *p;
    q = Pid;
    int i=0;
    for(i=0; i<20; i++)
    {
        p = (thpid *)malloc(sizeof(thpid));
        pthread_create(&p->id, NULL, fun, NULL);//创建线程池
        q -> next = p;
        p -> next = NULL;
        q = p;
    }
}

/*往工作任务列表中增加任务*/
void threadpools :: add_joblist(job *myjob)
{
    pthread_mutex_lock(&mutex);
    joblist.push_back(myjob);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

/*往线程池中添加线程*/
void threadpools :: add_threads()
{
    int all=0;
    thpid *stu;
    thpid *p = Pid;
    while(p)
    {
        p = p->next;
    }
    stu = (thpid *)malloc(sizeof(thpid));
    pthread_create(&stu->id, NULL, fun, NULL);
    p -> next = stu;
    stu -> next = NULL;
}

void* threadpools :: fun(void *arg)
{
    pthread_t id = pthread_self();
    while(1)
    {
        pthread_mutex_lock(&mutex);
        while(!flag && joblist.size()==0)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        /*退出线程*/
        if( flag )
        {
            pthread_mutex_unlock( &mutex);
            cout << id << "exit\n";
            pthread_exit(NULL);
        }
        /*取出一个工作任务*/
        vector <job *> :: iterator itr = joblist.begin();
        job *myjob = *itr;
        joblist.erase(itr);
        pthread_mutex_unlock(&mutex);
        int s = myjob->get_sockfd();
        int n = myjob->get_number();
        char *buf = myjob->get_buf();
        myjob->job_run(s, n, buf);
    }
}






