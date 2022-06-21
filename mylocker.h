//对同步机制进行包装
#ifndef MYLOCKER_H
#define MYLOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class mysem{
public:
    mysem(){
        if(sem_init(&m_sem,0,0)!=0)
            throw std::exception();
    }

    ~mysem(){
        sem_destroy(&m_sem);
    }

    bool wait(){
        return sem_wait(&m_sem)==0;
    }

    bool post(){
        return sem_post(&m_sem)==0;
    }

private:
    sem_t m_sem;
};

class mylocker{
public:
    mylocker(){
        if(pthread_mutex_init(&m_mutex,nullptr)!=0){
            throw std::exception();
        }
    }

    ~mylocker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }

private:
    pthread_mutex_t m_mutex;
};

class mycond{
public:
    mycond(){
        if(pthread_mutex_init(&m_mutex,nullptr)!=0)
            throw std::exception();
        if(pthread_cond_init(&m_cond,nullptr)!=0){
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }

    ~mycond(){
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    bool wait(){
        assert(pthread_mutex_lock(&m_mutex)==0);
        int ret=pthread_cond_wait(&m_cond,&m_mutex);
        assert(pthread_mutex_unlock(&m_mutex)==0);
        return ret==0;
    }

    bool signal(){
        return pthread_cond_signal(&m_cond)==0;
    }

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif
