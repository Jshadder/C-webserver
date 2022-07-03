//线程池
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <stdio.h>
#include <exception>
#include <pthread.h>
#include "mylocker.h"

template <typename T>
class threadpool{
public:
    threadpool(int thread_number=8,int max_requests=10000);
    ~threadpool();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();

private:
    int m_thread_number;        /*线程数量*/
    int m_max_requests;         /*允许请求最大数量*/
    pthread_t* m_threads;       /*线程池数组*/
    std::list<T*> m_workqueue;  /*工作队列*/
    mylocker m_queuelocker;     /*请求队列互斥锁*/
    mysem m_queuestat;          /*请求队列信号*/
    bool m_stop;                /*是否终止*/
};

template <typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):
m_thread_number(thread_number),m_max_requests(max_requests),m_stop(false),m_threads(nullptr){
    if(thread_number<=0||max_requests<=0){
        throw std::exception();
    }

    m_threads=new pthread_t[thread_number];
    
    for(int i=0;i<m_thread_number;++i){
        printf("Now creating thread %d.\n",i);

        if(pthread_create(m_threads+i,nullptr,worker,this)!=0){
            delete [] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i])!=0){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool(){
    delete [] m_threads;
    m_stop=true;
}

template <typename T>
bool threadpool<T>::append(T* request){
    assert(m_queuelocker.lock());
    if(m_workqueue.size()>=m_max_requests){
        assert(m_queuelocker.unlock());
        printf("Too many requests\n");
        return false;
    }
    m_workqueue.emplace_back(request);
    assert(m_queuelocker.unlock());
    assert(m_queuestat.post());
    return true;
}

template <typename T>
void* threadpool<T>::worker(void* arg){
    threadpool* obj=(threadpool*) arg;
    obj->run();
    return obj;
}

template <typename T>
void threadpool<T>::run(){
    while(!m_stop){
        while(!m_queuestat.wait());
        assert(m_queuelocker.lock());
        if(m_workqueue.empty()){
            assert(m_queuelocker.unlock());
            continue;
        }
        T* req=m_workqueue.front();
        m_workqueue.pop_front();
        assert(m_queuelocker.unlock());
        if(req==nullptr)
            continue;
        pthread_t tid=pthread_self();
        printf("Now thread %lu is serving\n",tid);
        req->process();
    }
}

#endif
