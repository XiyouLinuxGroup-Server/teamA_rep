/*************************************************************************
	> File Name: client.cpp
	> Author: hujialu
	> Mail: 
	> Created Time: 2018年02月28日 星期三 13时31分17秒
 ************************************************************************/

#include<iostream>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<assert.h>
#include<cstring>
#include<string>
#include<fcntl.h>
#include<stdlib.h>
using namespace std;

const char *ip = "127.0.0.1";
const int port = 4507;
int choice;

struct information{
    int number;
    char buff[256];
}; 

void dispaly()
{
    system("clear -l");
    cout << "\n\t\t\t根据以下功能输入选择\n";
    cout <<"\t\t\t(1)echo\n";
    cout << "\t\t\t(2)chargen\n";
    cout << "\t\t\t(3)time\n";
    cout << "\t\t\t(4)daytime\n";
    cout << "\t\t\t(5)discard\n";
    cout << "\t\t\t(6)退出\n";
}

class client
{
private:
    static int sock;
public:
    client();
    ~client();
    void init();
    void run();
    static void *recve(void *arg);
    static pthread_mutex_t mutex;
};

pthread_mutex_t client :: mutex = PTHREAD_MUTEX_INITIALIZER;
int  client :: sock = 0;

client::client()
{
    sock = -1;
}

client::~client()
{
    
}
void client::init()//初始化
{
    struct sockaddr_in address;
    int ret;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    //创建套接字
    sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock != -1);
    //建立链接
    ret = connect(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);
}

void client::run()//客户端选择运行
{
    cout << "\n\t\t\t" << endl;
    cin >> choice;
    struct information buffer;
    memset(&buffer,0,sizeof(struct information));
    switch(choice)
    {
        case 1://echo
        {
            buffer.number = 1;
            cin >> buffer.buff;
            break;
        }
        case 2://chargen
        {
            buffer.number = 2;
            break;
        }
        case 3://time
        {
            buffer.number = 3;
            break;
        }
        case 4://daytime
        {
            buffer.number = 4;
            break;
        }
        case 5://discard
        {
            buffer.number = 5;
            break;
        }
        default:
        {
            pthread_mutex_destroy(&mutex);
            exit(1);
        }
    }
    //send to server
    if(buffer.number > 0 && buffer.number < 6)
    {
        send(sock, (void *)&buffer, sizeof(buffer),MSG_WAITALL);
    }
    memset(&buffer, 0, sizeof(struct information));
}

void *client::recve(void *arg)
{
    int ret;
    int sock2 = sock;
    char buf[256];
    do{
        ret = recv(sock2,(void *)buf, 256, 0);
        assert(ret!=-1);
        if(ret <= 0)
        {
            break;
        }
        else{
            pthread_mutex_lock(&mutex);
            cout << buf << "\n";
            pthread_mutex_unlock(&mutex);
        }
    }while(1);
}

int main()
{
    pthread_t pid;
    client CL;
    CL.init();
    pthread_create(&pid, NULL , CL.recve , (void *)&CL);
    while(1)
    {
        dispaly();
        CL.run();
        if(choice==6)
        {
            break;
        }
    }
    pthread_mutex_destroy(&CL.mutex);
    return 0;
}
