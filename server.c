/*************************************************************************
	> File Name: server.c
	> Author: FengXin
	> Mail: fengxinlinux@gmail.com
	> Created Time: 2016年10月15日 星期六 11时09分58秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<sys/signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<time.h>
#include<errno.h>
#include"cJSON.h"



#define PORT  6666         //服务器端口
#define LISTEN_SIZE 1024   //连接请求队列的最大长度
#define EPOLL_SIZE  1024   //epoll监听客户端的最大数目



void my_err(const char*err_string,int line);   //自定义错误函数
char* my_time();      //输出当前时间函数
void contents_create();  //创建服务器初始化所需目录及文件



void my_err(const char *err_string,int line)  //自定义错误函数
{
    fprintf(stderr,"line:%d",line); //输出错误发生在第几行
    perror(err_string);       //输出错误信息提示
    exit(1);
}

char* my_time()    //输出当前时间函数
{
    time_t timep;
    time(&timep);
    char *p=ctime(&timep);
    return p;
}

void contents_create()  //创建服务器初始化所需目录及文件
{
    int fd;
    int n=0; //初始化建群和帖子的分配账号

    mkdir("mail_list",0777);  //创建主目录
    mkdir("mail_list/user",0777); //创建所有用户主目录
    mkdir("mail_list/group",0777);  //创建所有群主目录
    mkdir("mail_list/mail",0777);  //储存所有帖子目录

    fd=open("mail_list/allgroup",O_RDWR|O_CREAT|O_APPEND,0777); //创建储存当前所有的群信息的文件(smallgroup)
    close(fd);
    fd=open("mail_list/groupnum",O_RDWR|O_CREAT|O_APPEND,0777);//创建建群分配的账号
    if(write(fd,&n,sizeof(int))<0)
    {
        my_err("write",__LINE__);
    }
    close(fd);
    fd=open("mail_list/mailnum",O_RDWR|O_CREAT|O_APPEND,0777);//创建发帖分配的账号
    if(write(fd,&n,sizeof(int))<0)
    {
        my_err("write",__LINE__);
    }
    close(fd);

}


int main(int argc,char *argv[])
{
    int sock_fd;  //监听套接字
    int conn_fd;    //连接套接字
    int epollfd;  //epoll监听描述符
    socklen_t cli_len;  //记录连接套接字地址的大小
    struct epoll_event  event;   //epoll监听事件
    struct epoll_event*  events;  //epoll监听事件集合
    struct sockaddr_in cli_addr;  //客户端地址
    struct sockaddr_in serv_addr;   //服务器地址


    //创建一个套接字
    sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0)
    {
        my_err("socket",__LINE__);
    }
    //设置该套接字使之可以重新绑定端口
    int optval=1;
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(void*)&optval,sizeof(int))<0)
    {
        my_err("setsock",__LINE__);
    }
    //初始化服务器端地址结构
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(sock_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in))<0)
    {
        my_err("bind",__LINE__);
    }
    //将套接字转化为监听套接字
    if(listen(sock_fd,LISTEN_SIZE)<0)
    {
        my_err("listen",__LINE__);
    }

    contents_create(); //创建服务器初始化所需目录及文件

    cli_len=sizeof(struct sockaddr_in);
    events=(struct epoll_event*)malloc(sizeof(struct epoll_event)*EPOLL_SIZE); //分配内存空间
    
    //创建一个监听描述符epoll,并将监听套接字加入监听列表
    epollfd=epoll_create(EPOLL_SIZE);
    if(epollfd==-1)
    {
        my_err("epollfd",__LINE__);
    }
    event.events = EPOLLIN;
    event.data.fd = sock_fd;
    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,sock_fd,&event)<0)
    {
        my_err("epoll_ctl",__LINE__);
    }
     
    while(1)   //循环监听事件
    {
        int sum=0,i;
        sum=epoll_wait(epollfd,events,EPOLL_SIZE,-1);
        for(i=0;i<sum;i++)
        {
            if(events[i].data.fd==sock_fd)    //客户端请求连接
            {
                conn_fd=accept(sock_fd,(struct sockaddr*)&cli_addr,&cli_len);
                if(conn_fd<0)
                {
                    my_err("accept",__LINE__);
                }
                event.events = EPOLLIN | EPOLLRDHUP; //监听连接套接字的可读和退出
                event.data.fd = conn_fd;
                if(epoll_ctl(epollfd,EPOLL_CTL_ADD,conn_fd,&event)<0) //将新连接的套接字加入监听
                {
                    my_err("epoll",__LINE__);
                }
                printf("a connet is connected,ip is %s,time:%s",inet_ntoa(cli_addr.sin_addr),my_time());

            }
           /* else if(events[i].events&EPOLLIN)    //客户端发来数据
            {
                
            }  */
            if(events[i].events&EPOLLRDHUP) //客户端退出
            {
                printf("a connet is quit,ip is %s,time:%s",inet_ntoa(cli_addr.sin_addr),my_time());
                epoll_ctl(epollfd,EPOLL_CTL_DEL,events[i].data.fd,&event);
                close(events[i].data.fd);
            }
            
        }
    }

    return 0;

}



