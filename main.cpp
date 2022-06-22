#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "mylocker.h"
#include "threadpool.h"
#include "http_conn.h"

static const int MAX_FD=65536;
static const int MAX_EVENT_NUMBERE=10000;

extern int addfd(int epollfd,int fd,bool one_shot);
extern int removefd(int epollfd,int fd);

void addsig(int sig,void(*handler)(int),bool restart=true){

}

void show_error(int connfd,const char* info){

}

int main(int argc,char* argv[]){

}
