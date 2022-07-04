#include "timer_heap.h"

timer_heap::timer_heap(int cap):capacity(cap),cursize(0),arr(nullptr){
    arr=new heap_timer* [capacity];
    for(int i=0;i<capacity;++i)
        arr[i]=nullptr;
}

timer_heap::timer_heap(heap_timer** init_arr,int cap,int size):capacity(cap),cursize(size),arr(nullptr){
    if(capacity<=0||cursize<0)
        throw std::exception();
    if(capacity<cursize)
        throw std::exception();

    arr=new heap_timer* [capacity];
    for(int i=0;i<capacity;++i)
        arr[i]=nullptr;

    if(init_arr!=nullptr&&cursize>0){
        for(int i=0;i<cursize;++i){
            arr[i]=new heap_timer(*(init_arr[i]));
        }
        for(int i=(cursize-1)/2;i>=0;--i)
            percolate_down(i);
    }
}

timer_heap::~timer_heap(){
    for(int i=0;i<cursize;++i){
        delete arr[i];
        arr[i]=nullptr;
    }
    delete [] arr;
}

void timer_heap::add_timer(heap_timer* timer){
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
void timer_heap::del_timer(heap_timer* timer){
    if(timer==nullptr)
        return;
    timer->cb_func=nullptr;
}

void timer_heap::pop(){
    if(empty())
        return;
    if(arr[0]!=nullptr)
        delete arr[0];
    arr[0]=arr[--cursize];
    percolate_down(0);
}

void timer_heap::tick(){
    if(cursize==0)
        return;
    heap_timer* tmp=top();
    if(tmp==nullptr)
        return;
    time_t duetime=tmp->expire;

    while(tmp!=nullptr){
        if(tmp->expire>duetime)
            break;
        if(tmp->cb_func!=nullptr)
            tmp->cb_func(tmp->usr);
        pop();
        tmp=arr[0];
    }
}

void timer_heap::percolate_down(int hole){
    heap_timer* tmp=arr[hole];
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

void timer_heap::resize(){
    heap_timer** tmp=new heap_timer* [2*capacity];
    for(int i=0;i<2*capacity;++i)
        tmp[i]=nullptr;
    for(int i=0;i<cursize;++i)
        tmp[i]=arr[i];
    delete [] arr;
    capacity*=2;
    arr=tmp;
}