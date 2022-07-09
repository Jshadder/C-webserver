#include "myepoll.h"

int setnonblocking(int nfd)
{
    int nOldOpt=fcntl(nfd,F_GETFL);
    int nNewOpt=nOldOpt|O_NONBLOCK;
    fcntl(nfd,F_SETFL,nNewOpt);
    return nOldOpt;
}

myepoll::myepoll(int nsize,int nMaxNum)
{
    m_EpollFd=epoll_create(nsize);
    if(m_EpollFd<0)
    {
        throw std::exception();
    }
    m_Events=new epoll_event[nMaxNum];
    m_MaxEventNum=nMaxNum;
}

myepoll::~myepoll()
{
    close(m_EpollFd);
    delete [] m_Events;
}

void myepoll::AddFd(int nfd,bool oneshot)const
{
    epoll_event ev;
    ev.data.fd=nfd;
    ev.events=EPOLLET|EPOLLERR|EPOLLHUP|EPOLLRDHUP|EPOLLIN;
    if(true==oneshot)
    {
        ev.events|=EPOLLONESHOT;
    }
    epoll_ctl(m_EpollFd,EPOLL_CTL_ADD,nfd,&ev);
    setnonblocking(nfd);
}

void myepoll::ModFd(int nfd,int nev,bool oneshot)const
{
    epoll_event ev;
    ev.data.fd=nfd;
    ev.events=EPOLLET|EPOLLERR|EPOLLHUP|EPOLLRDHUP|nev;
    if(true==oneshot)
    {
        ev.events|=EPOLLONESHOT;
    }
    epoll_ctl(m_EpollFd,EPOLL_CTL_MOD,nfd,&ev);    
}

void myepoll::DelFd(int nfd)const
{
    epoll_ctl(m_EpollFd,EPOLL_CTL_DEL,nfd,nullptr);
    close(nfd);
}