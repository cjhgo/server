#include "public.h"
#include <pthread.h>
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int  parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void *thread(void *vargp);
int main(int argc, char const *argv[])
{
	
    int listenfd, *connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    /* Check command line args */
    if (argc != 2) 
    {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    port = atoi(argv[1]);
    
    listenfd = Open_listenfd(port);
    while (true) 
    {
       pthread_t tid;
     	 clientlen = sizeof(clientaddr);
       connfd=malloc(sizeof(int));
	     *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
       pthread_create(&tid,NULL,thread,connfd);
    }
}
void *thread(void *vargp)
{
    int connfd=*((int *) vargp);
    pthread_detach(pthread_self());
    free(vargp);
    doit(connfd);                                             //line:netp:tiny:doit
	  close(connfd);                                            //line:netp:tiny:close
}
void doit(int fd)
{
   int is_static;
   struct stat sbuf;
   char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
   char filename[MAXLINE],cgiargs[MAXLINE];
   rio_t rio;

   rio_readinitb(&rio,fd);
   //printf("doit\n");
   rio_readlineb(&rio,buf,MAXLINE);
   
   sscanf(buf,"%s %s %s",method,uri,version);
   //printf("buf is:%s\n", buf);
   if(strcasecmp(method,"GET"))
   {
   	  clienterror(fd,method,"501","NOT Implemented","my http server does't Implemented this method");
   	  return;
   }
   read_requesthdrs(&rio);   
   
   is_static=parse_uri(uri,filename,cgiargs);
   if(stat(filename,&sbuf)<0)
   {
   	clienterror(fd,filename,"404","NOT FOUND","my http server couldn't find the file");
   	return;
   }

   if(is_static)
   {
   	  //S_ISREG是否是一个常规文件
      if(!S_ISREG(sbuf.st_mode)||!(S_IRUSR & sbuf.st_mode))
      {
      	 clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      	return;
      }
      serve_static(fd,filename,sbuf.st_size);
    //  printf("static\n");
   }
   else
   {
      if(!S_ISREG(sbuf.st_mode)||!(S_IXUSR & sbuf.st_mode))
      {
      	clienterror(fd,filename,"403","Forbidden","Tiny couldn't run the file");
      	return;
      }
      serve_dynamic(fd,filename,cgiargs);
   }
   
}

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];
   //  printf("read_requesthdrs\n");
    rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}
int parse_uri(char *uri,char *filename,char *cgiargs)
{
	char *ptr;

	if(!strstr(uri,"cgi-bin"))
	{
      strcpy(cgiargs,"");
      strcpy(filename,".");
      strcat(filename,uri);
      if(uri[strlen(uri)-1]=='/')
        strcat(filename,"index.html");
      return 1;
	}
	else
	{
	 ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
   if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
   }
   else 
   strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
   strcpy(filename, "");                           //line:netp:parseuri:beginconvert2
   strcat(filename, uri+1);                           //line:netp:parseuri:endconvert2
   return 0;
	}
}


void serve_static(int fd, char *filename, int filesize) 
{
   int srcfd;
   char *srcp, filetype[MAXLINE], buf[MAXBUF];
   get_filetype(filename, filetype);
   sprintf(buf, "HTTP/1.0 200 OK\r\n");    //line:netp:servestatic:beginserve
   sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
   sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
   sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
   rio_writen(fd, buf, strlen(buf));  
   srcfd = open(filename, O_RDONLY, 0);
   srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
   close(srcfd);                           //line:netp:servestatic:close
   rio_writen(fd, srcp, filesize);         //line:netp:servestatic:write
   munmap(srcp, filesize);                 //line:netp:
}

void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
} 

void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) { /* child */ //line:netp:servedynamic:fork
  /* Real server would set all CGI vars here */
      setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
      Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
      Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}


