//日志阻塞队列
#ifndef _LOG_QUEUE_H_
#define _LOG_QUEUE_H_

#include <time.h>
#include "mylocker.h"

template <typename T>
class cBlockQueue
{
public:
    cBlockQueue(int maxsize=1000);
    
    bool Push(const T& message);

    bool Pop(T& message);

    bool IsEmpty()const{return m_nSize<=0;}

    bool IsFull()const{return m_nSize>=m_nMaxSize;}
private:
    int m_nMaxSize;
    int m_nSize;
    T* m_Array;
    int m_nFront;                       //队首
    int m_nBack;                        //队尾
    mycond m_Cond;                      //队列条件变量
};

template <typename T>
cBlockQueue<T>::cBlockQueue(int maxsize):m_nMaxSize(maxsize),m_nSize(0),m_nFront(-1),m_nBack(-1)
{
    if(m_nMaxSize<=0)
    {
        throw std::exception();
    }
    m_Array=new T[m_nMaxSize];
}

template <typename T>
bool cBlockQueue<T>::Push(const T& message)
{
    m_Cond.lock();
    if(true==IsFull())
    {
        m_Cond.signal();
        m_Cond.unlock();
        return false;
    }
    m_nBack=(m_nBack+1)%m_nMaxSize;
    m_Array[m_nBack]=message;
    if(m_nSize==0)
    {
        m_nFront=m_nBack;
    }
    ++m_nSize;
    m_Cond.signal();
    m_Cond.unlock();
    return true;
}

template <typename T>
bool cBlockQueue<T>::Pop(T& message)
{
    m_Cond.lock();
    while(IsEmpty())
    {
        if(false==m_Cond.wait())
        {
            m_Cond.unlock();
            return false;
        }
    }
    message=m_Array[m_nFront];
    m_nFront=(m_nFront+1)%m_nMaxSize;
    --m_nSize;
    m_Cond.unlock();
    return true;
}

#endif