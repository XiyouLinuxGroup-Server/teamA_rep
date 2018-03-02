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
#include<cstring>
#include<time.h>
#include<string>
#include<vector>
#include<cstdio>//perror的头文件

using namespace std;

const int MAxBuf = 100;
const int Min_Thread= 5;
const int Max_Thread = 10;
const int PORT = 45077;
const char *ip = "127.0.0.1";
const int sock_count = 256;
void my_error(string str,int line)
{
    fprintf(stderr,"line:%d ",line);
    perror(str.c_str());
}
class buffer
{
    public:
        int num;
        char buf[256];
        //如果使用string，是发送不了数据的
};
class my_task
{
    public:
        int sockfd;
        buffer buf;
        int num;
        void discard();
        void daytime();
        void my_time();
        void echo();
        void chargen();
        void run();
        int get_num();
        void set_data(int num,buffer tmp,int sock);
};
class thread_pool
{
    public:
        thread_pool();
        ~thread_pool();
        void create_thread();
        void stop_all();
        void add_task(my_task* task);
        static bool shutdown;   //标志着线程的退出
        static vector <my_task *> queue_list;  //任务列表
        static pthread_mutex_t mutex;
        static pthread_cond_t cond;

        pthread_t *pid;
        int init_thread;    //初始化线程数
        int now_thread;     //现在拥有的线程数
        int min_thread;   //应该保持的最少的线程数
        int max_thread;   //应该保持的最大空闲线程数
        static void * run_thread(void *arg);
};
bool thread_pool :: shutdown = false;
vector <my_task *> thread_pool  :: queue_list;
pthread_mutex_t thread_pool  :: mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_pool  :: cond = PTHREAD_COND_INITIALIZER;
thread_pool::thread_pool()
{
    init_thread = 5;
    now_thread = init_thread;
    min_thread = Min_Thread;
    max_thread = Max_Thread;
    pid = new pthread_t[ init_thread ];
    create_thread();
}
thread_pool::~thread_pool()
{
    delete[] pid;
    pid = nullptr;
}
void thread_pool::create_thread()
{
    for ( int i = 0; i < init_thread; i++ ) 
    {
        pthread_create( &pid[i], nullptr, run_thread, nullptr );
    }
}
void thread_pool::stop_all()
{
    if ( shutdown ) //避免重复调用
    {
        return ;
    }
    shutdown = true;
    pthread_cond_broadcast( &cond ); //唤醒所有线程
    for ( int i = 0; i < now_thread; i++ ) 
    {
        pthread_join( pid[i], nullptr );//pthread_join函数的调用者在等待子线程退出后才继续执行。
    }

    pthread_mutex_destroy( &mutex );
    pthread_cond_destroy( &cond );
}

void thread_pool::add_task(my_task* task)
{
    pthread_mutex_lock( &mutex );
    queue_list.push_back(task);
    pthread_cond_signal( &cond ); //任务添加后发出信号
    pthread_mutex_unlock( &mutex );
}
void *thread_pool::run_thread(void *arg)
{
    pthread_t tid = pthread_self();
    while (1) 
    {
        pthread_mutex_lock( &mutex );
        //如果没有任务且仍在正常运行　等待
        while ( !shutdown && queue_list.size() == 0 ) 
        {
            pthread_cond_wait( &cond, &mutex );//等待添加任务的唤醒
        }

        //退出线程
        if ( shutdown ) 
        {
            pthread_mutex_unlock( &mutex );
            cout << tid << " exit\n";
            pthread_exit( nullptr );
        }
        
        //取出一个任务
        vector <my_task *> :: iterator itr = queue_list.begin();
        my_task *job = *itr;
        queue_list.erase( itr ); //从任务队列删除取出的任务
        pthread_mutex_unlock( &mutex );
        job->run();
    }
}
void my_task::set_data(int num,buffer tmp,int sock)
{
    num = num;
    buf = tmp;
    sockfd = sock;
}
void my_task::discard()
{
    bzero(&buf,sizeof(buf));
}
void my_task::daytime()
{
    bzero(&buf, sizeof( buf ) );
    time_t secs;//用来存放自1970年1月1日0点0时0分开始的秒数。
    tm *local;
    secs = time( nullptr );
    local = localtime( &secs );//将秒转化
    strftime( (buf.buf), sizeof(buf.buf), "%Y-%m-%d %H:%M:%S", local ); //buf就是修改的时间
}
void my_task::my_time()
{
    time_t secs;
    secs = time( nullptr );
    char tmp[256];
    sprintf( tmp, "%d", (int)secs );//等于把秒变成字符串
    int mask = 256;//100000000
    int k = 0;
    for ( int i = 0; i < strlen(tmp); i++ ) 
    {
        for ( int j = 0; j < 9; j++ )//刚好循环到1
        {
            buf.buf[k++] = ( mask & (tmp[i]) ) ? 49 : 48; 
            //0或者1，等于每个位上的秒数都要转换为相应的二进制
            mask >>= 1;//每次1向右移动几位
        }
        mask = 256;
    }
}
void my_task::echo()//回显服务
{
    send( sockfd, (void *)&buf, sizeof(buf), 0 );//还是把原来数据发回去，然后读取的时候再去解析buf.buf
}

void my_task::chargen()
{
    srand((unsigned)time(NULL));
    int i =0 ;
    for(i=0;i< 30;i++)
        buf.buf[i] = rand();
    buf.buf[i] = '\0';//要成为字符串
}
int my_task::get_num()
{
    return buf.num;
}
void my_task::run()
{
     if(num == 1)
     {
         discard();
         send( sockfd, (void *)&buf, sizeof(buf), 0 );
     }
     else if(num == 2)
     {
        daytime();
        send( sockfd, (void *)&buf, sizeof(buf), 0 );
     }
     else if(num == 3)
     {
         my_time();
         send( sockfd, (void *)&buf, sizeof(buf), 0 );
     }
     else if(num == 4)
     {
         echo();
     }
     else if(num == 5)
     {
         chargen();
         while ( (send( sockfd, (void *)&buf, sizeof(buf), 0 ) ) != -1 ) 
        {
            sleep( 1 );
            chargen();
        }
     }
     else 
     {
         sprintf(buf.buf,"%s","输入有误！");
         send(sockfd,(void*)&buf,sizeof(buf),0);
     }
}
class my_epoll
{
   public:
        my_epoll();
        ~my_epoll();
        int setnonblocking(int sockfd);
        void init();
        //int epoll_accept();//接受客户端
        void epoll_addfd(int epollfd,int sockfd,bool oneshot);//添加到内核事件中
        void epoll_recv(int sockfd);
        void epoll_run();
    private:
        int sock;
        int epfd;
        thread_pool *pool;
        my_task task;
        struct epoll_event ev, *event;  //ev用于处理事件　event数组用于回传要处理的事件
};

my_epoll::my_epoll()
{
    sock = 0;
    epfd = 0;
    pool = new thread_pool;
    event = new epoll_event[30];//事件
}
my_epoll::~my_epoll()
{
    delete pool;
    delete [] event;
    pool = nullptr;
    event = nullptr;
}
int my_epoll::setnonblocking(int sockfd)
{
    int old_option = fcntl(sockfd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sockfd,F_SETFL,new_option);
    return old_option;
}
void my_epoll::init()
{
    int ret;
    struct sockaddr_in address;
    bzero( &address, sizeof(address) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( PORT );
   
    sock = socket( PF_INET, SOCK_STREAM, 0 );   
    if(sock < 0)
    {
        my_error("socket",__LINE__);
    }
    
    ret = bind( sock, (struct sockaddr*)&address, sizeof( address ) );
    if(sock < 0)
    {
        my_error("bind",__LINE__);
    }
    ret = listen( sock, 5 );//监听
     if(sock < 0)
    {
        my_error("listen",__LINE__);
    }

    epfd = epoll_create( 5 );
    cout <<"链接成功！！！"<<endl;//////////////////
}
void my_epoll::epoll_addfd(int epollfd,int sockfd,bool oneshot)
{
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;
    if(oneshot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,sockfd,&event);
    setnonblocking(sockfd);
}
 void my_epoll::epoll_recv(int sockfd)
 {
    buffer buf;
    int ret;
    if ( (ret = recv( sockfd, (void *)&buf, sizeof(buf), 0 ) <= 0 ))  
    {
        ev.data.fd = sockfd;
        ev.events = EPOLLERR;   //对应文件描述符发生错误
        epoll_ctl( epfd, EPOLL_CTL_DEL, sockfd, &ev );
        close( sockfd ); //断开连接
        return ;
    }
    task.set_data( buf.num, buf, sockfd );
    pool->add_task( &task ); //往线程池添加任务
 }
 void my_epoll::epoll_run()
 {
    int file_num;
    while (1) 
    {
        file_num =epoll_wait( epfd, event, 20, -1 );   //-1表示一直等下去直到有时间发生 为任意正整数的时候表示等这么久
        if ( file_num == 0 ) 
        {
            cout << "Time Out\n";
            continue;
        }
        else if ( file_num == -1 ) 
        {
            cout << "Error\n";
        }
        else 
        {
            for ( int i = 0; i < file_num; i++ )  //处理发生的所有事
            {
                if ( event[i].data.fd == sock )     //有新的客户端连接
                {
                    sockaddr_in client_addr;
                    bzero( &client_addr, sizeof(client_addr) );
                    socklen_t length = sizeof( client_addr );
                    int connfd = accept(sock,(struct sockaddr*)&client_addr,&length);//接受客户端
                    epoll_addfd(epfd,connfd,true);//添加到内核事件中
                }
                
                else if ( event[i].events & EPOLLIN ) //有可读事件 
                {
                    epoll_recv( event[i].data.fd );
                }
                else
                {
                    cout <<"something else happened" <<endl;
                }
                
            }
        }
        bzero( event, sizeof(event) );  //清空event数组
    }
 }

 int main()
 {
    my_epoll epoll;
    epoll.init();
    epoll.epoll_run();
 }