
#include <stdio.h>
#include"net.h"



FILE* logfp=NULL;
int islog=0;
int isdebug=0;
char cgi_path[512]= {'\0'};


void logging(const char* logfilename)
{
    logfp=fopen(logfilename,"a+");
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void *accept_request(void* tclient)
{

    char buf[1024];
    int numchars;
    struct arg_struct*args=(struct arg_struct*)tclient;
    int client =args->socket_id;
    char method[255];
    char url[255];
    char path[512];
    char time1[100];
    size_t i, j;
    struct stat st;
    int cgi = 0;      // becomes true if server decides this is a CGI
    // program
    char *query_string = NULL;

    time_t   now;         //实例化time_t结构
    struct   tm  *timenow;         //实例化tm结构指针
    time(&now);
    timenow   =   localtime(&now);
    strcpy(time1,asctime(timenow));
    if(islog)
    {
        fwrite(time1,strlen(time1),1,logfp);
        fflush(logfp);
        printf("信息写入日志入log\n");
//localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
    }
    if(isdebug)
    {
        printf("%s",time1);
    }
    //printf("the client is %d\n",client);
    numchars = get_line(client, buf, sizeof(buf));
    //printf("%d",numchars);
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s\n",buf);
    i = 0;
    j = 0;
   // printf("%s",buf);
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';
    //printf("method is :%s\n",method);
    fflush(stdout);

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return NULL;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    //printf("1.this is url: %s\n",url);
    //fflush(stdout);

    int k1=0;

    if(url[0]!='\0')
    {
        int k2=0;
        char tool[8];
        for(; k2<=7; k2++)
        {
            tool[k2]=url[k2];
        }
        if(strcmp(tool,"/cgi-bin")==0)
        {

            while(url[k1+k2]!='\0')
            {
                url[k1]=url[k1+k2];
                k1++;
            }
            url[k1]='\0';
            //printf("5555..:url of cgi_dir: %s\n",url);
            //fflush(stdout);
        }
    }

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    if(args->dir[0]=='\0')
    {
        sprintf(path, "htdocs%s", url);
        //printf("3333.:%s\n",path);
        //fflush(stdout);
    }
    else{
        sprintf(path, "%s%s", args->dir,url);
        //printf("4444.:%s\n",path);
        //fflush(stdout);
    }
    //sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    if (stat(path, &st) == -1)
    {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;
        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
    }
// request.getRemoteAddr()

    close(client);
    return NULL;
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s",buf);
//
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
//content-length
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s",buf);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;
    int respo_byte=0;

    buf[0] = 'A';
    buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
    else    /* POST */
    {
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        if (content_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s",buf);
    send(client, buf, strlen(buf), 0);

    if (pipe(cgi_output) < 0)
    {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return;
    }

    if ( (pid = fork()) < 0 )
    {
        cannot_execute(client);
        return;
    }
    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else     /* POST */
        {
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }

        /***********option -c dir*************/

        execl(path, path, NULL);
        exit(0);
    }
    else        /* parent */
    {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++)
            {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
        {
            send(client, &c, 1, 0);
            respo_byte++;
        }
        /*char*(respo_byte);
        if(islog)
         {
           fwrite(respo_byte,strlen(respo_byte),1,logfp);
          }
        if(isdebug)
          printf("%s ",respo_byte);
        */
        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s\n",buf);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s\n",buf);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port,int tag,char address[])
{
    int httpd = 0;
    struct sockaddr_in name;

    memset(&name, 0, sizeof(name));
    name.sin_port = htons(*port);
//ipv6部分
    if(tag==1)
    {
        httpd = socket(PF_INET6, SOCK_STREAM, 0);
        if (httpd == -1)
            error_die("socket");
        name.sin_family = AF_INET6;
        inet_pton(AF_INET6,address, (void *)& name.sin_addr.s_addr);
    }
//ipv4
    else if(tag==0)
    {
        httpd = socket(PF_INET, SOCK_STREAM, 0);
        if (httpd == -1)
            error_die("socket");
        name.sin_family = AF_INET;
        name.sin_addr.s_addr = inet_addr(address);
    }
    else
    {
        httpd = socket(PF_INET, SOCK_STREAM, 0);
        if (httpd == -1)
            error_die("socket");
        name.sin_family = AF_INET;
        name.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    if (*port == 0)  /* if dynamically allocating a port */
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 5) < 0)
        error_die("listen");
    return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
//
    if(islog)
        fwrite(buf,strlen(buf),1,logfp);
    fflush(logfp);
    if(isdebug)
        printf("%s\n",buf);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


/*****************************************************/


void print_usage()
{
    printf("−c dir Allow execution of CGIs from the given directory. See    CGIs for details.\n");
    printf("−d Enter debugging mode. That is, do not daemonize, only accept one connection at a time and enable logging to stdout.\n");
    printf("−h Print a short usage summary and exit.\n");
    printf("−i address Bind to the given IPv4 or IPv6 address. If not provided, sws will listen on all IPv4 and IPv6 addresses on this host.\n");
    printf("−l file Log all requests to the given file. See LOGGING for details.\n");
    printf("−p port Listen on the given port. If not provided, sws will listen on port 8080.\n");
}



