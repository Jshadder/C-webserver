#ifndef TIMER_HEAP
#define TIMER_HEAP

#include <iostream>
#include <netinet/in.h>
#include <time.h>

template <class T>
class heap_timer{
public:
    heap_timer(){
        expire=-1;
        cb_func=nullptr;
        usr=nullptr;
    }

    heap_timer(time_t delay,void (*f)(T*)=nullptr,T* u=nullptr):cb_func(f),usr(u){
        expire=time(nullptr)+delay;
    }
public:
    time_t expire;
    void (*cb_func)(T*);
    T* usr;
};

template <class T>
class timer_heap{
public:
    timer_heap(int cap);
    timer_heap(heap_timer<T>** init_arr,int cap,int size);
    ~timer_heap();

public:
    void add_timer(heap_timer<T>* timer);
    void del_timer(heap_timer<T>* timer);
    heap_timer<T>* top() const{return empty()?nullptr:arr[0];};
    void pop();
    void tick();
    bool empty() const{return cursize==0;};

private:
    void percolate_down(int hole);
    void resize();

private:
    heap_timer<T>** arr;
    int capacity;
    int cursize;
};

template <class T>
timer_heap<T>::timer_heap(int cap):capacity(cap),cursize(0),arr(nullptr){
    arr=new heap_timer<T>* [capacity];
    for(int i=0;i<capacity;++i)
        arr[i]=nullptr;
}

template <class T>
timer_heap<T>::timer_heap(heap_timer<T>** init_arr,int cap,int size):capacity(cap),cursize(size),arr(nullptr){
    if(capacity<=0||cursize<0)
        throw std::exception();
    if(capacity<cursize)
        throw std::exception();

    arr=new heap_timer<T>* [capacity];
    for(int i=0;i<capacity;++i)
        arr[i]=nullptr;

    if(init_arr!=nullptr&&cursize>0){
        for(int i=0;i<cursize;++i){
            arr[i]=init_arr[i];
        }
        for(int i=(cursize-1)/2;i>=0;--i)
            percolate_down(i);
    }
}

template <class T>
timer_heap<T>::~timer_heap(){
    for(int i=0;i<cursize;++i)
        delete arr[i];
    delete [] arr;
}

template <class T>
void timer_heap<T>::add_timer(heap_timer<T>* timer){
    if(timer==nullptr)
        return;
    
    if(cursize>=capacity)
        resize();

    int hole=cursize++;
    int parent=0;
    while(hole>0){
        parent=(hole-1)/2;
        if(arr[parent]->expire<=timer->expire)
            break;
        arr[hole]=arr[parent];
        hole=parent;
    }
    arr[hole]=timer;
}

//懒惰删除
template <class T>
void timer_heap<T>::del_timer(heap_timer<T>* timer){
    if(timer==nullptr)
        return;
    timer->cb_func=nullptr;
}

template <class T>
void timer_heap<T>::pop(){
    if(empty())
        return;
    arr[0]=arr[--cursize];
    percolate_down(0);
}

template <class T>
void timer_heap<T>::tick(){
    if(cursize==0)
        return;
    heap_timer<T>* tmp=top();
    if(tmp==nullptr)
        return;
    time_t duetime=time(nullptr);

    while(tmp!=nullptr){
        if(tmp->expire>duetime)
            break;
        if(tmp->cb_func!=nullptr)
            tmp->cb_func(tmp->usr);
        pop();
        if(empty())
            break;
        tmp=arr[0];
    }
}

template <class T>
void timer_heap<T>::percolate_down(int hole){
    heap_timer<T>* tmp=arr[hole];
    int child=0;
    while(2*hole+1<cursize){
        child=2*hole+1;
        if((child+1<cursize)&&(arr[child+1]->expire<arr[child]->expire))
            ++child;
        if(arr[child]->expire>=tmp->expire)
            break;
        arr[hole]=arr[child];
        hole=child;
    }
    arr[hole]=tmp;
}

template <class T>
void timer_heap<T>::resize(){
    heap_timer<T>** tmp=new heap_timer<T>* [2*capacity];
    for(int i=0;i<2*capacity;++i)
        tmp[i]=nullptr;
    for(int i=0;i<cursize;++i)
        tmp[i]=arr[i];
    delete [] arr;
    capacity*=2;
    arr=tmp;
}

#endif