#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<stdio.h>
using namespace std;

int a = 0;
int b = 0;

pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void *another(void *arg)
{
    //给互斥量b上锁
    pthread_mutex_lock(&mutex_b);
    cout << "in child thread got mutex b,waiting for mutex a\n";
    sleep(5);
    ++b;
    cout << "in child thread watite mutex_a\n";
    pthread_mutex_lock(&mutex_a);
    b += a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    pthread_exit(NULL);
}

int main()
{
    pthread_t id;

    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);
   
    //执行子线程
    pthread_create(&id, NULL, another, NULL);
    
    //主线程给互斥量a上锁
    pthread_mutex_lock(&mutex_a);
    cout << "in parent thread ,got mutex a,waiting for mutex b\n";
    sleep(5);
    ++a;
    cout << "in parent thread waite mutex_b\n";
    pthread_mutex_lock(&mutex_b);
    a += b++;
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    pthread_join(id, NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);
    return 0;
}

