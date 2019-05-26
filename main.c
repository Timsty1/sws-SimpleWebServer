

#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include"net.h"

int main(int argc,char *argv[])
{

 int server_sock = -1;
 u_short port = 8080;
 int client_sock = -1;
 struct sockaddr_in client_name;
 socklen_t client_name_len = sizeof(client_name);
 pthread_t newthread;
 struct arg_struct args;

 //char cgi_path1[255]={'\0'};
 char address[255]={'\0'};
 char file[255]={'\0'};
 char port_char[36]={'\0'};
 int opt;
 char addr_client[255];
 const char *opstring="c:dhi::l::p::";
 int tag=-1;
 int count=0;

 while((opt = getopt(argc,argv,opstring))!=-1)
 {
  switch(opt){
   case'c':strcpy(args.dir,optarg);break;
   case'd':isdebug=1;break;
   case'h':print_usage();exit(1);break;
   case'i':strcpy(address,optarg);break;
   case'p':strcpy(port_char,optarg);port=(u_short)atoi(port_char);break;
   case'l':strcpy(file,optarg); logging(file);islog=1;break;
  }
 }
 if(isdebug==1)
  islog=0;
//处理CGI路径
 //strcpy(cgi_path,cgi_path1);

//address修改
 int k=0;
 printf("%s",address);
 while(address[k]!='\0')
 {
  k++;
  if(address[k]=='.')
   {tag=0;break;}
  else if(address[k]==':')
   {tag=1;break;}
 }

//port修改
 server_sock = startup(&port,tag,address);
 printf("httpd running on port %d\n", port);

  while(1)
{
  client_sock = accept(server_sock,
                       (struct sockaddr *)&client_name,
                       &client_name_len);
  args.socket_id=client_sock;
  strcpy(addr_client,inet_ntoa(client_name.sin_addr));
  printf("the client IP is:%s \n",addr_client);
  if(islog)
    {fwrite(addr_client,strlen(addr_client),1,logfp);
    fflush(logfp);}
  if (client_sock == -1)
   error_die("accept");
 /* accept_request(client_sock); */
  if (pthread_create(&newthread , NULL, accept_request, (void *)&args) != 0)
   perror("pthread_create");
 if(islog)
  fflush(logfp);
}
 close(server_sock);
 if(islog)
  fclose(logfp);
 return (0);

}






















