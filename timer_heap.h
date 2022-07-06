#ifndef TIMER_HEAP
#define TIMER_HEAP

#include <iostream>
#include <netinet/in.h>
#include <time.h>


template <class T>
class heap_timer{
typedef void (*cbfunp)(T*);

public:
    heap_timer():expire(-1),cb_func(nullptr),usr(nullptr),idx(-1){}

    heap_timer(time_t delay,cbfunp f=nullptr,T* u=nullptr):cb_func(f),usr(u),idx(-1){
        expire=time(nullptr)+delay;
    }

public:
    time_t get_expire()const{return expire;}
    void delay_expire(time_t delay){expire=time(nullptr)+delay;}
    cbfunp get_cbfunc()const{return cb_func;}
    void set_cbfunc(cbfunp f){cb_func=f;}
    T* get_usr()const{return usr;}
    void set_usr(T* u){usr=u;}
    int get_idx()const{return idx;}
    void set_idx(int i){idx=i;}

private:
    time_t expire;
    cbfunp cb_func;
    T* usr;
    int idx;
};

template <class T>
class timer_heap{
typedef void (*cbfunp)(T*);

public:
    timer_heap(int cap);
    timer_heap(heap_timer<T>** init_arr,int cap,int size);
    ~timer_heap();

public:
    void add_timer(heap_timer<T>* timer);
    void delay_timer(heap_timer<T>* timer,time_t delay);
    void del_timer(heap_timer<T>* timer);
    heap_timer<T>* top() const{return empty()?nullptr:arr[0];}
    void pop();
    void tick();
    bool empty() const{return cursize==0;}

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
        if(arr[parent]->get_expire()<=timer->get_expire())
            break;
        arr[hole]=arr[parent];
        arr[hole]->set_idx(hole);
        hole=parent;
    }
    arr[hole]=timer;
    timer->set_idx(hole);
}

//更新的时间影响定时精度
template <class T>
void timer_heap<T>::delay_timer(heap_timer<T>* timer,time_t delay){
    if(timer==nullptr)
        return;
    if(delay<=0)
        throw std::exception();
    timer->delay_expire(delay);
    percolate_down(timer->get_idx());
}

//懒惰删除
template <class T>
void timer_heap<T>::del_timer(heap_timer<T>* timer){
    if(timer==nullptr)
        return;
    timer->set_cbfunc(nullptr);
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
        if(tmp->get_expire()>duetime)
            break;
        if(tmp->get_cbfunc()!=nullptr)
            tmp->get_cbfunc()(tmp->get_usr());
        pop();
        tmp=top();
    }
}

template <class T>
void timer_heap<T>::percolate_down(int hole){
    heap_timer<T>* tmp=arr[hole];
    int child=0;
    while(2*hole+1<cursize){
        child=2*hole+1;
        if((child+1<cursize)&&(arr[child+1]->get_expire()<arr[child]->get_expire()))
            ++child;
        if(arr[child]->get_expire()>=tmp->get_expire())
            break;
        arr[hole]=arr[child];
        arr[hole]->set_idx(hole);
        hole=child;
    }
    arr[hole]=tmp;
    arr[hole]->set_idx(hole);
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
