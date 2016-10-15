/*************************************************************************
	> File Name: client.c
	> Author: FengXin
	> Mail: fengxinlinux@gmail.com
	> Created Time: 2016年10月15日 星期六 14时57分22秒
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
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<time.h>
#include"cJSON.h"

#define PORT 6666

void my_err();  //自定义错误函数



void my_err(char* err_string,int line)      //自定义错误函数
{
    fprintf(stderr,"line:%d",line); //输出错误发生在第几行
    perror(err_string);       //输出错误信息提示
    exit(1);
}

int main(int argc,char* argv[])
{
    int conn_fd; //创建连接套接字
    struct sockaddr_in serv_addr; //储存服务器地址

    if(argc!=3)    //检测输入参数个数是否正确
    {
        printf("Usage: [-a] [serv_address]\n");
        exit(1);
    }


    //初始化服务器地址结构
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);

    //从命令行服务器地址
    for(int i=0;i<argc;i++)
    {
        if(strcmp("-a",argv[i])==0)
        {
            
            if(inet_aton(argv[i+1],&serv_addr.sin_addr)==0)
            {
                printf("invaild server ip address\n");
                exit(1);
            }
            break;
        }
    }

    //检查是否少输入了某项参数
    if(serv_addr.sin_addr.s_addr==0)
    {
        printf("Usage: [-a] [serv_address]\n");
        exit(1);
    }

    //创建一个TCP套接字
    conn_fd=socket(AF_INET,SOCK_STREAM,0);


    if(conn_fd<0)
    {
        my_err("connect",__LINE__);
    }

    //向服务器发送连接请求
    if(connect(conn_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0)
    {
        my_err("connect",__LINE__);
    }






}
