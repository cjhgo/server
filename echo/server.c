
#include "public.h"
void echo(int connfd);

int main(int argc, char const *argv[])
{
	int listenfd,connfd,port,clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp;
	if(argc!=2)
    {
        fprintf(stderr, "usage: %s<host> <port>\n", argv[0]);
        return 0;
    }
    port=atoi(argv[1]);
    
    listenfd=open_listenfd(port);
    printf("server is open and listening port: %d",port);
    while(1)
    {
       clientlen=sizeof(clientaddr);
       connfd=accept(listenfd,(SA*)&clientaddr, &clientlen);

       hp=gethostbyaddr((const char*)&clientaddr.sin_addr.s_addr,
       	               sizeof(clientaddr.sin_addr.s_addr),AF_INET);
       haddrp=inet_ntoa(clientaddr.sin_addr);
       printf("server connected to %s (%s)\n",hp->h_name,haddrp);

       echo(connfd);
       close(connfd);
    }
}
//服务器只用对特定端口进行监听
int open_listenfd(int port)
{
    int listenfd,optval=1;
    struct sockaddr_in serveraddr; //sockaddr_in 存放套接字地址，
    //建立socket，
    if((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        return -1;
    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,
                 (const void*)&optval,sizeof(int)) < 0)
        return -1;

    bzero((char *)&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serveraddr.sin_port=htons((unsigned short)port);

    if(bind(listenfd,(SA*)&serveraddr,sizeof(serveraddr))<0)
        return -1;
    if(listen(listenfd,LISTENQ)<0)
        return -1;
    return listenfd;
}

void echo(int connfd)
{
	size_t n;
	char buf[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio,connfd);
	while((n=Rio_readlineb(&rio,buf,MAXLINE))!=0)
	{
      printf("server received %d bytes\n",n);
      Rio_writen(connfd,buf,n);
	}
}
