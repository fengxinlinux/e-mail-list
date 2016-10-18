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
#include<dirent.h>
#include<pwd.h>
#include<errno.h>
#include"cJSON.h"



#define PORT  6666         //服务器端口
#define LISTEN_SIZE 1024   //连接请求队列的最大长度
#define EPOLL_SIZE  1024   //epoll监听客户端的最大数目

/* 
 flag标志位代表的意义:
 1=登陆，11=登陆成功，111=登陆失败；
 2=注册，22=注册成功，222=注册失败;
 */


typedef struct     //记录用户账号信息
{
    char account[21]; //账号
    char passwd[21];  //密码
}user;



void my_err(const char*err_string,int line);   //自定义错误函数
char* my_time();      //输出当前时间函数
void my_send(int conn_fd,char* send_buf,int len); //自定义发送函数
void my_recv(int conn_fd,char* recv_buf,int len);  //自定义接收函数
void contents_create();  //创建服务器初始化所需目录及文件
void add_len(char* out,char* send_buf);  //在数据包前加两个字节的包长度函数
char* delete_len(char* recv_buf,int len);    //将数据包前两个字节删除函数
void deal_main(int conn_fd,int flag,char* out);    //主功能处理函数
void server_recv(int conn_fd);    //服务器接收并解析数据包函数
void login(int conn_fd,char* out);   //登陆函数
void register_(int conn_fd,char* out);  //注册函数





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

void my_send(int conn_fd,char* send_buf,int buf_len)   //自定义发送函数
{
    if(send(conn_fd,send_buf,buf_len,0)<0)
    {
        my_err("send",__LINE__);
    }
}

void my_recv(int conn_fd,char* recv_buf,int buf_len)  //自定义接收函数
{
    int len; //储存接收数据大小
    int ret; //记录recv函数返回值
    int sum=0;  //储存已接收的字节大小

    
    if((ret=recv(conn_fd,recv_buf,buf_len,0))<0) 
    {
        my_err("recv",__LINE__);
    }
    len=(unsigned char)recv_buf[0]+256*(unsigned char)recv_buf[1];
    sum+=ret;
    /*while(sum!=len)
    {
    
        if((ret=recv(conn_fd,recv_buf+sum,len-sum,0))<0)
        {
            my_err("recv",__LINE__);
        }
        sum+=ret;
    } */ 

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

void add_len(char* out,char* send_buf)  //在数据包前加两个字节的包长度函数
{
    int len=strlen(out)+3;  //储存数据包长度
    

    send_buf[0]=len&0xff;  //低8位储存在send_buf第一个字节
    send_buf[1]=(len>>8)&0xff;  //高8位储存在send_buf的第二个字节
    strcpy(send_buf+2,out);   //将json数据包里的内容复制到send_buf中

    

}

char* delete_len(char* recv_buf,int len) //将数据包前两个字节删除函数
{
    char *out; //储存json字符串
    len=len-2;
    out=(char*)malloc(len);  //分配空间
    strcpy(out,recv_buf+2);  //将数据包里的json字符串复制到out中

    return out;
}

void deal_main(int conn_fd,int flag,char *out)    //主功能处理函数
{
    switch(flag)
    {
        case 1:
        {
            login(conn_fd,out);   //登陆函数
            break;
        }
        case 2:
        {
            register_(conn_fd,out);  //注册函数
            break;
        }
    }
}
void server_recv(int conn_fd)    //服务器接收并解析数据包函数
{

    char recv_buf[1000];   //储存接收的json形式的数据
    char *out;  //储存json字符串
    cJSON * json_flag;  //储存标志位
    cJSON * json;  //json根对象
    
    my_recv(conn_fd,recv_buf,sizeof(recv_buf));   //接收数据包
    out=delete_len(recv_buf,sizeof(recv_buf));    //去掉包长度数据
    json=cJSON_Parse(out); //解析成cjson形式
    json_flag=cJSON_GetObjectItem(json,"flag");   //获得服务器返回标志位

    deal_main(conn_fd,json_flag->valueint,out);  //主功能处理函数

}

void login(int conn_fd,char* out)   //登陆函数
{
    cJSON * json;  //json根对象
    cJSON * json_account;  //储存账号的json
    cJSON * json_passwd;  //储存密码的json
    user number1; //储存接收到的用户账号与密码
    user number2;  //储存从文件读出的用户账号与密码
    int ret=0; //记录是否登陆成功，若成功则为1;
    DIR* dp;   //储存目录打开返回值
    struct dirent * dir;  //储存目录信息
    char send_buf[1000];  //发送数据的储存空间
    char path[PATH_MAX+1]; //记录文件打开路径
    int fd;    //文件描述符
    

    json=cJSON_Parse(out); //解析成cjson形式
    json_account=cJSON_GetObjectItem(json,"account");  //获得账号json
    json_passwd=cJSON_GetObjectItem(json,"passwd");    //获得密码json

   

    strcpy(number1.account,json_account->valuestring);//将账号复制到结构体中
    strcpy(number1.passwd,json_passwd->valuestring);  //将密码复制到结构体中

  
    cJSON_Delete(json); //释放内存
    free(out);

    if((dp=opendir("mail_list/user"))==NULL)  //打开用户目录
    {
        my_err("opendir",__LINE__);
    }
    while((dir=readdir(dp))!=NULL)
    {
        if(strcmp(number1.account,dir->d_name)==0) //用户文件夹已存在
        {
            strcpy(path,"mail_list/user/");
            strcat(path,number1.account);
            strcat(path,"/userinfo");
            fd=open(path,O_RDWR);
            if(fd<0)
            {
                my_err("open",__LINE__);
            }
            if(read(fd,&number2,sizeof(user))<0)
            {
                my_err("read",__LINE__);
            }
            
            printf("json_account=%s,json_passwd=%s\n",json_account->valuestring,json_passwd->valuestring);///////////
            if(strcmp(number2.account,number1.account)==0&&strcmp(number2.passwd,number1.passwd)==0) //匹配成功
            {
                ret=1;
                break;
            }
        }
    }
    if(ret) //登陆成功
    {
        json=cJSON_CreateObject(); //创建根数据对象
        cJSON_AddNumberToObject(json,"flag",11); //加入标志位键值
        out=cJSON_Print(json);  //打印json
        add_len(out,send_buf);  //头部加入长度
        my_send(conn_fd,send_buf,sizeof(send_buf));  //发送数据
    }
    else   //登陆失败
    {
        json=cJSON_CreateObject(); //创建根数据对象
        cJSON_AddNumberToObject(json,"flag",111); //加入标志位键值
        out=cJSON_Print(json);  //打印json
        add_len(out,send_buf);  //头部加入长度
        my_send(conn_fd,send_buf,sizeof(send_buf));  //发送数据

    }

}

void register_(int conn_fd,char* out)  //注册函数
{
    cJSON * json;  //json根对象
    cJSON * json_account;  //储存账号的json
    cJSON * json_passwd;  //储存密码的json
    user number;   //储存用户账号与密码
    int ret=0;   //记录注册账号是否存在，若存在则为1
    DIR* dp;   //储存目录打开返回值
    struct dirent * dir;  //储存目录信息
    char send_buf[1000];  //发送数据的储存空间
    char path[PATH_MAX+1]; //记录文件打开路径
    int fd; //文件描述符


    

    json=cJSON_Parse(out); //解析成cjson形式
    json_account=cJSON_GetObjectItem(json,"account");  //获得账号json
    json_passwd=cJSON_GetObjectItem(json,"passwd");    //获得密码json

    strcpy(number.account,json_account->valuestring); //将账号复制到结构体中
    strcpy(number.passwd,json_passwd->valuestring);  //将密码复制到结构体中
    cJSON_Delete(json); //释放内存
    free(out);

    

    if((dp=opendir("mail_list/user"))==NULL)  //打开用户目录
    {
        my_err("opendir",__LINE__);
    }
    while((dir=readdir(dp))!=NULL)
    {
        if(strcmp(number.account,dir->d_name)==0) //用户文件夹已存在
        {
            ret=1;
            break;
        }
    }
    if(ret)  //用户已被注册
    {
        json=cJSON_CreateObject(); //创建根数据对象
        cJSON_AddNumberToObject(json,"flag",222); //加入标志位键值
        out=cJSON_Print(json);  //打印json
        add_len(out,send_buf);  //头部加入长度
        my_send(conn_fd,send_buf,sizeof(send_buf));  //发送数据
    }
    else //用户未被注册
    {
        strcpy(path,"mail_list/user/");
        strcat(path,number.account);  //合成用户目录所在路径
        mkdir(path,0777);  //创建用户目录

        strcat(path,"/userinfo");//用户基本信息
        fd=open(path,O_CREAT|O_RDWR|O_APPEND,0777); //创建用户基本信息文件
        if(fd<0)
        {
            my_err("open",__LINE__);
        }
        if(write(fd,&number,sizeof(user))<0)  //将账号信息写入文件中
        {
            my_err("write",__LINE__);
        }
        close(fd);

        strcpy(path,"mail_list/user/");
        strcat(path,number.account); //合成用户目录所在路径
        strcat(path,"/group");     //用户所加讨论组文件
        fd=open(path,O_RDWR|O_CREAT|O_APPEND,0777);
        close(fd);

        strcpy(path,"mail_list/user/");
        strcat(path,number.account); //合成用户目录所在路径
        strcat(path,"/mailqueue");   //有回帖新动态的消息队列
        fd=open(path,O_RDWR|O_CREAT|O_APPEND,0777);
        close(fd);

        strcpy(path,"mail_list/user/");
        strcat(path,number.account); //合成用户目录所在路径
        strcat(path,"/usermail");     //存放用户参与的帖子编号及名字
        fd=open(path,O_RDWR|O_CREAT|O_APPEND,0777);
        close(fd);


        json=cJSON_CreateObject(); //创建根数据对象
        cJSON_AddNumberToObject(json,"flag",22); //加入标志位键值
        out=cJSON_Print(json);  //打印json
        add_len(out,send_buf);  //头部加入长度
        my_send(conn_fd,send_buf,sizeof(send_buf));  //发送数据

    }
    
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
    char recv_buf[1000];   //储存接收的json形式的数据包


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
            else if(events[i].events&EPOLLIN)    //客户端发来数据
            {
                
                server_recv(events[i].data.fd);  //接收数据包并做处理
                
            }
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



