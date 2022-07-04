#ifndef TIMER_HEAP
#define TIMER_HEAP

#include <iostream>
#include <netinet\in.h>
#include <time.h>
#include "http_conn.h"

class heap_timer{
public:
    heap_timer(time_t delay){
        expire=time(nullptr);
    }
public:
    time_t expire;
    void (*cb_func)(http_conn*);
    http_conn* usr;
};

class timer_heap{
public:
    timer_heap(int cap);
    timer_heap(heap_timer** init_arr,int cap,int size);
    ~timer_heap();

public:
    void add_timer(heap_timer* timer);
    void del_timer(heap_timer* timer);
    heap_timer* top() const{return empty()?nullptr:arr[0];};
    void pop();
    void tick();
    bool empty() const{return cursize==0;};

private:
    void percolate_down(int hole);
    void resize();

private:
    heap_timer** arr;
    int capacity;
    int cursize;
};

#endif