#include<iostream>
#include<pthread.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;
int count = 0;
//全局变量是对多个同步运行的线程共享的
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
//初始化，或者是pthread_init()
void* thread_son(void *arg);
int main()
{
    pthread_t idone,idtwo;
    for(int i=0;i < 10;i++)//count++ 20次
    {
        pthread_create(&idone,NULL,thread_son,NULL);
        pthread_create(&idtwo,NULL,thread_son,NULL);

        pthread_join(idone,NULL);
        pthread_join(idtwo,NULL);
        //在Linux中，默认情况下是在一个线程被创建后，必须使用此函数对创建的线程进行资源回收

        pthread_mutex_destroy(&counter_mutex);
    }

}
void* thread_son(void * arg)
{
    pthread_mutex_lock(&counter_mutex);
    count++;
    cout <<"count is "<<count<<endl;
    pthread_mutex_unlock(&counter_mutex);
}