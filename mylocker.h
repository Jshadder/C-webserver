//对同步机制进行包装
#ifndef MYLOCKER_H
#define MYLOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class mysem{
public:
    mysem(){

    }

    ~mysem(){

    }

    bool wait(){

    }

    bool post(){

    }

private:
    sem_t m_sem;
};

class mylocker{
public:
    mylocker(){

    }

    ~mylocker(){

    }

    bool lock(){

    }

    bool unlock(){

    }

private:
    pthread_mutex_t m_mutex;
};

class mycond{
public:
    cond(){

    }

    ~cond(){

    }

    bool wait(){

    }

    bool signal(){

    }

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};