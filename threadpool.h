//线程池
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
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
    locker m_queuelocker;       /*请求队列互斥锁*/
    sem m_queuestat;            /*请求队列信号*/
    bool m_stop;                /*是否终止*/
};

template <typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):
m_thread_number(thread_number),m_max_requests(max_requests),m_stop(false),m_threads(nullptr){

}

template <typename T>
threadpool<T>::~threadpool(){

}

template <typename T>
bool threadpool<T>::append(T* request){

}

template <typename T>
void* threadpool<T>::worker(void* arg){

}

template <typename T>
void threadpool<T>::run(){

}

#endif