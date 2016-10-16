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

#define PORT 6666   //服务器端口



/*
 flag标志位的代表意义：
 1=登陆，11=登陆成功，111=登陆失败;
 2=注册，22=注册成功，333=注册失败;
*/
  


typedef struct     //记录用户账号信息
{
    char account[21]; //账号
    char passwd[21];  //密码
}user;




char* add_len(char* out);  //在数据包前加两个字节的包长度函数
char* delete_len(char* recv_buf);    //将数据包前两个字节删除函数
void login(int conn_fd);    //登陆函数
void register(int conn_fd);  //注册函数
void login_register(int conn_fd); //登陆与注册主界面函数
void my_err();  //自定义错误函数
void my_send(int conn_fd,char* send_buf); //自定义发送函数
void my_recv(int conn_fd,char* recv_buf);  //自定义接收函数
void menu_main(); //主菜单函数


char* add_len(char* out)  //在数据包前加两个字节的包长度函数
{
    int len=sizeof(out);  //储存数据包长度
    char* send_buf;  //储存发送的数据包
    send_buf=(char*)malloc(len+2);   //分配加长度后的数据空间

    send_buf[0]=len&0xff;  //低8位储存在send_buf第一个字节
    send_buf[1]=(len>>8)&0xff;  //高8位储存在send_buf的第二个字节
    strcpy(send_buf+2,out);   //将json数据包里的内容复制到send_buf中

    return send_buf;

}
char* delete_len(char* recv_buf) //将数据包前两个字节删除函数
{
    int len=sizeof(recv_buf)-2; //储存json字符串长度
    char *out; //储存json字符串
    out=(char*)malloc(len);  //分配空间
    strcpy(out,recv_buf+2);  //将数据包里的json字符串复制到out中

    return out;
}
void login(int conn_fd)  //登陆函数
{
    char account[50]; //储存用户输入的账号
    char passwd[50];  //储存用户输入的密码
    int len;      //记录输入的长度
    cJSON *usr;  //json储存用户账号对象
    char *out;  //储存json打印字符串
    char *send_buf; //储存发送数据包
    char recv_buf[1000];  //储存接收数据包
    int flag;  //标志位

    system("clear"); //清屏
    printf("请输入账号:");
    scanf("%s\n",account);
    while((len=strlen(accout))>20) //输入长度大于20
    {
        printf("账号不能超过20个字符\n");
        scanf("%s\n",accout);
    }
    printf("请输入密码:");
    scanf("%s\n",passwd);
    while((len=strlen(account))>0)
    {
        printf("密码不能超过20个字符\n");
        scanf("%s\n",passwd);
    }

    usr=cJSON_CreateObject(); //创建根数据对象
    cJSON_AddNumberToObject(usr,"flag",1); //加入标志位键值
    cJSON_AddStringToObject(usr,"account",account); //加入账号键值
    cJSON_AddStringToObject(usr,"passwd",passwd);  //加入密码键值
    out=cJSON_Print(usr);  //打印json

    send_buf=add_len(out);
    free(out); //释放内存
    cJSON_Delete(json);  //释放内存
    my_send(conn_fd,send_buf); //发送数据
    my_recv(conn_fd,recv_buf); //接收数据
    out=delete_len(recv_buf); //得到json字符串

    json=cJSON_Parse(out);  //解析成cjson形式
    flag=cJSON_GetObjectItem(json,"flag"); //获得服务器返回的标志位

    switch(flag)
    {
        case 11:   //登陆成功
        {
            printf("登陆成功,%s欢迎回来\n",account);
            sleep(2);  //停留2秒
            return;
        }
        case 111:  //登陆失败
        {
            printf("账号或密码错误\n")；
            login_register(conn_fd);
            break;
        }
    }   

}
void register(int conn_fd) //注册函数
{
    char accout[21];  //储存用户输入的账号
    char passwd[21];  //储存用户输入的密码
    char passwd2[21]; //储存用户第二次输入的密码
    int len;   //记录输入的长度
    cJSON *usr;   //json储存用户账号对象
    char *out;    //储存json打印字符串
    char *send_buf;   //储存发送数据包
    char recv_buf[1000];   //储存接收数据包
    int flag;  //标志位
    


}
void my_err(char* err_string,int line)      //自定义错误函数
{
    fprintf(stderr,"line:%d",line); //输出错误发生在第几行
    perror(err_string);       //输出错误信息提示
    exit(1);
}

void my_send(int conn_fd,char* send_buf)   //自定义发送函数
{
    if(send(conn_fd,send_buf,sizeof(send_buf),0)<0)
    {
        my_err("send",__LINE__);
    }
}
void my_recv(int conn_fd,char* recv_buf)  //自定义接收函数
{
    int len; //储存接收数据大小
    int ret; //记录recv函数返回值
    int sum=0;  //储存已接收的字节大小
    
    if((ret=recv(conn_fd,recv_buf,sizeof(recv_buf),0))<0)
    {
        my_err("recv",__LINE__);
    }
    len=(unsigned char)recv_buf[0]+256*(unsigned char)recv_buf[1];
    sum+=ret;
    while(sum!=len)
    {
        if((ret=recv(conn_fd,recv_buf+sum,len-sum,0))<0)
        {
            my_err("recv",__LINE__);
        }
        sum+=ret;
    }

}

void login_register(int conn_fd)  //登陆与注册主界面函数
{
    char choice[20];  //用字符串储存用户选项

    printf("******欢迎使用西邮校内邮件列表系统******\n");
    printf("              1.登陆\n");
    printf("              2.注册\n");
    printf("              3.退出\n");
    printf("请输入选项:");

    while(1)  //储存用户选项
    {
        int j=0;
        while(j!=20&&(choice[j++]=getchar())!='\n'); //储存用户输入选项
        if(j==20)
        {
          printf("您输入的选项过长，请重新输入\n");
          continue;
        }
        else
        {
            choice[--j]='\0';
            if(strcmp(choice,"1")!=0&&strcmp(choice,"2")!=0&&strcmp(choice,"3")!=0)  //输入选项不正确，重新输入
            {
                printf("您输入的选项不正确,请重新输入\n");
                continue;
            }
            else  //输入选项正确，退出循环
            {
                break;
            }
        }
    }

    switch(*choice)
    {
        case '1':    //登陆
        {
            login(conn_fd); 
            break;
            
        }
        case '2':    //注册
        {}
        case '3':   //退出系统
        {
            exit(0);
            break;
        }
    }

}

void menu_main()   //主菜单函数
{
    char choice[20];   //用字符串储存用户选项

    system("clear"); //清屏
    printf("              1.创建邮件列表\n");
    printf("              2.加入邮件列表\n");
    printf("              3.我的邮件列表\n");
    printf("              4.新的动态回复\n");
    printf("              5.写新的公开邮件\n");
    printf("              6.查看所有公开邮件\n");
    printf("              7.退出\n");
    printf("请输入选项:%s",choice);

    while(1)  //储存用户选项
    {
        int j=0;
        while(j!=20&&(choice[j++]=getchar())!='\n'); //储存用户输入选项
        if(j==20)
        {
            printf("您输入的选项过长，请重新输入\n");
            continue;        
        }
        else
        {
            choice[--j]='\0';
            if(strcmp(choice,"1")!=0&&strcmp(choice,"2")!=0&&strcmp(choice,"3")!=0)  //输入选项不正确，重新输入
            {
                printf("您输入的选项不正确,请重新输入\n");
                continue;             
            }
            else  //输入选项正确，退出循环
            {
                break;    
            }   
        }
    }
    
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


    login_register(conn_fd); //登陆与注册函数
    
    while(1)
    {
        menu_main(); //主菜单函数
    }






}
