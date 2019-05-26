
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>


#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
struct arg_struct {
    char dir[255];
    int socket_id;
};


void *accept_request(void* tclient);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
//int startup(u_short *);
int startup(u_short *port,int tag,char address[]);
void unimplemented(int);

void logging(const char* logfilename);
void print_usage();

extern FILE* logfp;
extern int islog;
extern int isdebug;
extern char cgi_path[512];





