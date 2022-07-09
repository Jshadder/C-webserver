#ifndef _MYEPOLL_H_
#define _MYEPOLL_H_

#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>
#include <exception>
#include <unistd.h>

int setnonblocking(int nfd);

class myepoll
{
public:
    myepoll(int nsize=100,int nMaxNumber=10000);

    ~myepoll();

public:
    void AddFd(int nfd,bool oneshot)const;

    void ModFd(int nfd,int nev,bool oneshot)const;

    void DelFd(int nfd)const;

    int Wait(time_t timeout=-1){return epoll_wait(m_EpollFd,m_Events,m_MaxEventNum,timeout);}

    int GetFd(int idx)const{return m_Events[idx].data.fd;}

    int GetEvent(int idx)const{return m_Events[idx].events;}

private:
    int m_EpollFd;
    epoll_event* m_Events;
    int m_MaxEventNum;
};

#endif