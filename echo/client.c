#include "public.h"
int open_clientfd(char* hostname,int port);
int open_listenfd(int port);

int main(int argc, char const *argv[])
{
    int clientfd,port;
    char *host,buf[MAXLINE];
    rio_t rio;
    if(argc!=3)
    {
        fprintf(stderr, "usage: %s<host> <port>\n", argv[0]);
        return 0;
    }
    host=argv[1];
    port=atoi(argv[2]);

    clientfd=open_clientfd(host,port);
    Rio_readinitb(&rio,clientfd);
    
    while(Fgets(buf,MAXLINE,stdin)!=NULL)
    {
        Rio_writen(clientfd,buf,strlen(buf));
        Rio_readlineb(&rio,buf,MAXLINE);
        Fputs(buf,stdout);
    }
    close(clientfd);
    return 0;

}
//客户端：传入服务器的主机名，以及服务器的端口
int open_clientfd(char* hostname,int port)
{
    int clientfd;
    struct hostent *hp; //hostten是主机条目结构，可以认为每条主机条目就是一个域名+IP地址的等价类
    struct sockaddr_in serveraddr; //sockaddr_in 存放套接字地址，
    //在客户端建立socket，失败则小于0
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0))<0) //clientfd为套接字表述符
      return -1;
    //获取该域名对应的主机条目，若不存在则返回NULL
    if((hp=gethostbyname(hostname))==NULL)
      return -2;
    //可以理解成 初始化server_addr
    bzero((char*)&serveraddr,sizeof(serveraddr));

    serveraddr.sin_family = AF_INET; //表明是因特网
    //将主机目录的第一个IP地址拷贝到服务器套接字地址结构
    bcopy((char*)hp->h_addr_list[0],
          (char*)&serveraddr.sin_addr.s_addr,
          hp->h_length);

    serveraddr.sin_port = htons(port);
    //connect函数将试图与套芥子为serveraddr的服务器建立一个因特网连接，
    if(connect(clientfd, (SA*)&serveraddr,sizeof(serveraddr))<0)
     return -1;
    return clientfd;
}
