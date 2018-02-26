#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdlib.h>//perror的头文件
using namespace std;
const char *ip = "127.0.0.1";
const int port = 45077;
void my_error(string str,int line)
{
    fprintf(stderr,"line:%d ",line);
    perror(str.c_str());
}
class client
{
    public:
        int sock;
        int ret;
        pthread_mutex_t mutex;
        client();
        ~client(){}
        void display();
        void init();
        void send_from_client();
        static void* recv_from_server(void *arg);
        //相当于全局函数，非静态函数会在列表中加上一个this指针，和参数不符合
};
class buffer
{
    public:
        int num;
        char buf[256];
        //如果使用string，是发送不了数据的
};
client::client()
{
    sock = 0;
    ret = 0;
    mutex = PTHREAD_MUTEX_INITIALIZER;
}
void client::display()
{
    system("clear");
    cout <<"\t\t\t\t欢迎使用"<<endl;
    cout <<"\t\t1、discard"<<endl;
    cout <<"\t\t2、daytime"<<endl;
    cout <<"\t\t3、time"<<endl;
    cout<<"\t\t4、echo"<<endl;
    cout<<"\t\t5、chargen"<<endl;
    cout <<"\t\t请输入你的选项"<<endl;
}
void client::init()
{
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&addr.sin_addr);
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr=inet_addr("127.0.0.1");//服务器地址
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0)//创建客户端套接字
    {
        my_error("client socket",__LINE__);
    }
    if((ret=connect(sock,(struct sockaddr *)&addr,sizeof(struct sockaddr)))<0)//将套接字绑定到服务器地址
    {
       my_error("connect",__LINE__);
    }
}
void client::send_from_client()
{   
    buffer data;
    string in;
    while(1)
    {
        display();
        cin >> in;
        if(in =="1")
        {
            data.num = 1;
            break;
        }
        else if(in =="2")
        {
            data.num = 2;
            break;
        }
        else if(in == "3")
        {
            data.num =3;
            break;
        }
        else if(in =="4")
        {
            data.num =4;
            cout <<"请输入你要发送的数据;";
            cin >> data.buf;
            break;
        }
        else 
        {
            data.num = 0;
            cout <<"输入有误，请重新输入"<<endl;
            sleep(2);
        }
        if(data.num!=0)
        {
            send(sock,(void*)&data,sizeof(data),0);
        }
        bzero(&data,sizeof(data));
    }
    

}
void* client::recv_from_server(void *arg)
{
    char buf[256];
    client &tmp =*(client*)arg;
    while(1)//一直循环是因为不知道什么时候接受数据
    {
        //->优先级高，所以（client*）arg->ret的时候，arg还是void*
        tmp.ret = recv(tmp.sock,(void*)buf,sizeof(buf),0);
        if(tmp.ret <=0)
        {
            break;
        }
        cout <<buf<<endl;
    }
}
int main()
{
    pthread_t pid;
    client test;
    test.init();
    test.send_from_client();
    pthread_create(&pid,nullptr,test.recv_from_server,(void*)&test);
    //将对象传进去
    pthread_mutex_destroy(&test.mutex);

    return 0;
}