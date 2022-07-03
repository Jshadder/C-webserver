#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <libgen.h>

static const char* request="Get http://localhost/a.file HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxxx";

int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    int new_opt=old_opt|O_NONBLOCK;
    assert(fcntl(fd,F_SETFL,new_opt)==0);
    return old_opt;
}

void addfd(int epollfd,int fd){
    epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLOUT|EPOLLET|EPOLLERR|EPOLLRDHUP|EPOLLHUP;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnonblocking(fd);
}

void modfd(int epollfd,int fd,int event){
    epoll_event ev;
    ev.data.fd=fd;
    ev.events=event|EPOLLRDHUP|EPOLLHUP|EPOLLERR|EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}

bool write_nbytes(int sockfd,const char* buffer,int len){
    printf("write out %d bytes to socket %d\n",len,sockfd);
    while(true){
        int ret=send(sockfd,buffer,len,0);
        if(ret<=0)
            return false;
        len-=ret;
        if(len<=0)
            return true;
        buffer+=ret;
    }
}

bool read_once(int sockfd,char* buffer,int len){
    memset(buffer,'\0',len);
    int ret=recv(sockfd,buffer,len,0);
    if(ret<=0)
        return false;
    printf("read %d bytesa from socket %d with content: %s\n",ret,sockfd,buffer);
    return true;
}

void start_conn(int epollfd,int num,const char* ip,int port){
    sockaddr_in addr;
    socklen_t addrlen=sizeof(addr);
    __bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    inet_pton(AF_INET,ip,&addr.sin_addr);
    addr.sin_port=htons(port);

    for(int i=0;i<num;++i){
        sleep(1);
        int curfd=socket(AF_INET,SOCK_STREAM,0);
        if(curfd<0)
            continue;
        printf("create socket %d\n",curfd);
        if(connect(curfd,(sockaddr*)&addr,addrlen)<0){
            printf("Fail to connect socket %d to server\n",curfd);
            close(curfd);
            continue;
        }
        addfd(epollfd,curfd);

    }
}

void close_conn(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,nullptr);
    close(fd);
}

int main(int argc,char* argv[]){
    if(argc<4){
        printf("usage: %s ip port num",basename(argv[0]));
        return 1;
    }
    int epollfd=epoll_create(100);
    assert(epollfd>=0);
    epoll_event events[10000];
    start_conn(epollfd,atoi(argv[3]),argv[1],atoi(argv[2]));

    char rdbuf[2048];
    while(true){
        int number=epoll_wait(epollfd,events,10000,2000);
        if(number<0){
            printf("epoll failuer\n");
            break;
        }
        for(int i=0;i<number;++i){
            int curfd=events[i].data.fd;
            if(events[i].events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR))
                close_conn(epollfd,curfd);
            else if(events[i].events&EPOLLOUT){
                if(!write_nbytes(curfd,request,strlen(request))){
                    close_conn(epollfd,curfd);
                    continue;
                }
                modfd(epollfd,curfd,EPOLLIN);
            }
            else if(events[i].events&EPOLLIN){
                if(!read_once(curfd,rdbuf,2048)){
                    close_conn(epollfd,curfd);
                    continue;
                }
                modfd(epollfd,curfd,EPOLLOUT);
            }
            else{
                printf("unknown event\n");
            }
        }
    }
    close(epollfd);
}