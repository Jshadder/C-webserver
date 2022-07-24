#ifndef _MY_LOG_H_
#define _MY_LOG_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include "logqueue.h"


class Log
{
public:
    static Log* GetInstance();                      //懒汉模式

    static void* FlushLogThread(void* args){GetInstance()->AsyncWriteLog();};         //静态异步写日志的工作函数

    bool Init(const char* szFileName,               //初始化
    int nCloseLog,
    int nLogBufSize=8192,
    int nSplitLines=500000,
    int nMaxQueueSize=100);

    void WriteLog(int nLevel,const char* format,...);

    void flush(void){fflush(m_FP);};                               //刷新缓冲

    bool IsCloseLog()const{return 0!=m_nCloseLog;}

private:
    Log(){};

    ~Log();

    void AsyncWriteLog();                           //实际的工作函数

public:
    enum LogLevel{DEBUG=0,INFO,WARN,ERROR};

private:
    char DirName[128];
    char LogName[128];
    int m_nSplitLines;
    int m_nLogBufSize;
    long long m_nCountLines;
    int m_nToday;
    FILE* m_FP;
    char* m_Buf;
    cBlockQueue<std::string>* m_LogQueue;
    bool m_IsAsync;
    mylocker m_MutexLock;
    int m_nCloseLog;
    bool m_WriteStop;
};

#define LOG_DEBUG(format,...) if(!Log::GetInstance()->IsCloseLog()){Log::GetInstance()->WriteLog(Log::DEBUG,format,##__VA_ARGS__);Log::GetInstance()->flush();}
#define LOG_INFO(format,...) if(!Log::GetInstance()->IsCloseLog()){Log::GetInstance()->WriteLog(Log::INFO,format,##__VA_ARGS__);Log::GetInstance()->flush();}
#define LOG_WARN(format,...) if(!Log::GetInstance()->IsCloseLog()){Log::GetInstance()->WriteLog(Log::WARN,format,##__VA_ARGS__);Log::GetInstance()->flush();}
#define LOG_ERROR(format,...) if(!Log::GetInstance()->IsCloseLog()){Log::GetInstance()->WriteLog(Log::ERROR,format,##__VA_ARGS__);Log::GetInstance()->flush();}

#endif