#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/epoll.h>
#include <libgen.h>

#include "mylocker.h"
#include "threadpool.h"
#include "http_conn.h"
#include "timer_heap.h"

static const int TIMEOUT=20;
static const int MAX_FD=65536;
static const int MAX_EVENT_NUMBERE=10000;
static timer_heap<http_conn> TH(1000);
static int pipefd[2];//信号管道

extern int addfd(int epollfd,int fd,bool one_shot);
extern int removefd(int epollfd,int fd);
extern int setnonblocking(int fd);

void sig_handler(int sig){
    int saveno=errno;
    int msg=sig;
    send(pipefd[1],(char*)&msg,1,0);
    errno=saveno;
}

void addsig(int sig,void(*handler)(int),bool restart=true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler=handler;
    if(restart){
        sa.sa_flags|=SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr)==0);
}

void show_error(int connfd,const char* info){
    printf("%s",info);
    send(connfd,info,strlen(info),0);
    close(connfd);
}

void time_handler(){
    TH.tick();
    if(!TH.empty())
        alarm(TH.top()->get_expire()-time(nullptr));
}

void cb_func(http_conn* usr){
    printf("closing silent socket\n");
    usr->close_conn();
}

int main(int argc,char* argv[]){
    if(argc<=2){
        printf("usage: %s ip_address port_number\n",basename(argv[0]));
        return 1;
    }
    const char* ip_address=argv[1];
    int port_number=atoi(argv[2]);
    //忽略SIGPIPE
    addsig(SIGPIPE,SIG_IGN);
    addsig(SIGALRM,sig_handler);
    addsig(SIGTERM,sig_handler);
    //addsig(SIGSTOP,sig_handler);
    //close(STDIN_FILENO);
    //close(STDOUT_FILENO);
    //close(STDERR_FILENO);
    threadpool<http_conn>* workpool=nullptr;
    try{
        workpool=new threadpool<http_conn>(16,10000);
    }
    catch(...){
        printf("Fail to new threadpool\n");
        return 1;
    }


    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    inet_pton(AF_INET,ip_address,&addr.sin_addr);
    addr.sin_port=htons(port_number);
    
    int listenfd=socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd>=0);
    struct linger lin={1,0};//设置socket丢弃写缓冲数据并强制断开连接
    setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&lin,sizeof(lin));

    int ret=0;

    ret=bind(listenfd,(sockaddr*)&addr,sizeof(addr));
    assert(ret>=0);

    ret=listen(listenfd,10);
    assert(ret>=0);

    int epollfd=epoll_create(10);
    assert(epollfd>=0);
    addfd(epollfd,listenfd,false);
    http_conn::m_epollfd=epollfd;
    epoll_event epollevent[MAX_EVENT_NUMBERE];

    http_conn* users=new http_conn[MAX_FD];

    ret=socketpair(AF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret==0);
    setnonblocking(pipefd[1]);
    addfd(epollfd,pipefd[0],false);

    bool stop=false;
    bool timeout=false;

    while(!stop){
        int number=epoll_wait(epollfd,epollevent,MAX_EVENT_NUMBERE,-1);
        if(number<0){
            if(errno!=EAGAIN&&errno!=EINTR){
                printf("epoll wait failure\n");
                break;
            }
        }

        for(int i=0;i<number;++i){
            int curfd=epollevent[i].data.fd;
            if(curfd==listenfd){
                struct sockaddr_in client_addr;
                socklen_t client_len=sizeof(client_addr);
                while(true){
                    int connfd=accept(curfd,(sockaddr*)&client_addr,&client_len);
                    if(connfd<0){
                        if(errno==EAGAIN)
                            break;
                        printf("accept() errno is:%d\n",errno);
                        break;
                    }  

                    if(http_conn::m_user_count>=MAX_FD){
                        show_error(connfd,"Too many users!\n");
                        continue;
                    }

                    users[connfd].init(connfd,client_addr);
                    users[connfd].timer=heap_timer<http_conn>(TIMEOUT,cb_func,users+connfd);//每个连接保活TIMEOUT秒
                    if(TH.empty())
                        alarm(users[connfd].timer.get_expire()-time(nullptr));
                    TH.add_timer(&users[connfd].timer);
                }
            }
            else if((curfd==pipefd[0])&&(epollevent[i].events&EPOLLIN)){
                int sig=0;
                char signals[1024];
                ret=recv(curfd,signals,sizeof(signals),0);
                if(ret<=0){
                    printf("read signals failure\n");
                    continue;
                }
                else{
                    for(int i=0;i<ret;++i){
                        sig=signals[i];
                        switch (sig){
                        case SIGALRM:{
                            timeout=true;//定时器任务优先级不高
                            break;
                        }
                        case SIGSTOP:
                        case SIGTERM:{
                            stop=true;
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            else if(epollevent[i].events&(EPOLLHUP|EPOLLERR|EPOLLRDHUP)){
                TH.del_timer(&users[curfd].timer);
                users[curfd].close_conn();
            }
            else if(epollevent[i].events&EPOLLIN){
                if(users[curfd].read()){
                    workpool->append(users+curfd);
                    TH.delay_timer(&users[curfd].timer,TIMEOUT);
                }
                else{
                    TH.del_timer(&users[curfd].timer);
                    users[curfd].close_conn();
                }
            }
            else if(epollevent[i].events&EPOLLOUT){
                if(!users[curfd].write()){
                    TH.del_timer(&users[curfd].timer);
                    users[curfd].close_conn();
                }
            }
        }

        if(timeout){
            time_handler();
            timeout=false;
        }
    }
    close(listenfd);
    close(epollfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    delete workpool;
    return 0;
}
