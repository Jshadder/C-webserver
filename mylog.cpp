#include "mylog.h"

Log *Log::GetInstance()
{
    static Log OneLog;
    return &OneLog;
}

bool Log::Init(const char *szFileName,
               int nCloseLog,
               int nLogBufSize,
               int nSplitLines,
               int nMaxQueueSize)
{
    m_WriteStop=false;
    m_nSplitLines = nSplitLines;
    m_nCloseLog = nCloseLog;
    m_nCountLines = 0;

    m_nLogBufSize = nLogBufSize;
    m_Buf = new char[m_nLogBufSize];
    memset(m_Buf, '\0', m_nLogBufSize);

    time_t CurTime = time(nullptr);
    struct tm *SysTm = localtime(&CurTime);
    m_nToday = SysTm->tm_mday;

    if (nMaxQueueSize <= 0)
    {
        m_IsAsync = false;
    }
    else
    {
        m_IsAsync = true;
        m_LogQueue = new cBlockQueue<std::string>(nMaxQueueSize);
        pthread_t tid;
        if (0 != pthread_create(&tid, nullptr, Log::FlushLogThread, nullptr))
        {
            throw std::exception();
        }
        printf("Write thread created\n");
        if(0!=pthread_detach(tid))
        {
            throw std::exception();
        }
    }

    const char *Name = strrchr(szFileName, '/');
    char FullName[256] = {'\0'};
    if (nullptr == Name)
    {
        strncpy(LogName, szFileName, 128);
        LogName[127] = '\0';
        DirName[0] = '\0';
    }
    else
    {
        strncpy(LogName, Name + 1, 128);
        LogName[127] = '\0';
        strncpy(DirName, szFileName, Name - szFileName + 1);
        DirName[127] = '\0';
    }
    snprintf(FullName, 255, "%s%d_%02d_%02d_%s", DirName, SysTm->tm_year + 1900, SysTm->tm_mon + 1, m_nToday, LogName);

    m_FP = fopen(FullName, "a");
    if (nullptr == m_FP)
    {
        printf("Log open failed\n");
        return false;
    }
    return true;
}

void Log::WriteLog(int nlevel, const char *format, ...)
{
    char szPrefix[16];
    switch (nlevel)
    {
        case Log::DEBUG:
        {
            strncpy(szPrefix, "[DEBUG]:", 16);
            szPrefix[15] = '\0';
            break;
        }
        case Log::INFO:
        {
            strncpy(szPrefix, "[INFO]:", 16);
            szPrefix[15] = '\0';
            break;
        }
        case Log::WARN:
        {
            strncpy(szPrefix, "[WARN]:", 16);
            szPrefix[15] = '\0';
            break;
        }
        case Log::ERROR:
        {
            strncpy(szPrefix, "[ERROR]:", 16);
            szPrefix[15] = '\0';
            break;
        }
        default:
        {
            strncpy(szPrefix, "[UNKNOWN]:", 16);
            szPrefix[15] = '\0';
            break;
        }
    }

    time_t CurTime = time(nullptr);
    struct tm *SysTm = localtime(&CurTime);

    m_MutexLock.lock();

    ++m_nCountLines;

    //如果不是今天的日志或者行数是最大行数整数倍时，分文件
    if(SysTm->tm_mday!=m_nToday||m_nCountLines%m_nSplitLines==0)
    {
        flush();
        fclose(m_FP);

        char NewLog[256]={'\0'};
        char Tail[20]={'\0'};
        snprintf(Tail,20,"%d_%02d_%02d_",SysTm->tm_year+1900,SysTm->tm_mon+1,SysTm->tm_mday);

        if(SysTm->tm_mday!=m_nToday)
        {
            snprintf(NewLog,255,"%s%s%s",DirName,Tail,LogName);
            m_nToday=SysTm->tm_mday;
            m_nCountLines=0;
        }
        else
        {
            snprintf(NewLog,255,"%s%s%s-%d",DirName,Tail,LogName,m_nCountLines/m_nSplitLines+1);
        }
        m_FP=fopen(NewLog,"a");
        if(nullptr==m_FP)
        {
            throw std::exception();
        }
    }

    m_MutexLock.unlock();

    va_list ValList;
    va_start(ValList,format);
    std::string LogStr;

    //格式化输出：时间+内容
    m_MutexLock.lock();

    int nCntTime=snprintf(m_Buf,m_nLogBufSize-2,"%d-%02d-%02d,%02d:%02d:%02d %s\n",
    SysTm->tm_year+1900,SysTm->tm_mon+1,m_nToday,SysTm->tm_hour,SysTm->tm_min,SysTm->tm_sec,szPrefix);

    int nCntContent=vsnprintf(m_Buf+nCntTime,m_nLogBufSize-2-nCntTime,format,ValList);
    m_Buf[nCntTime+nCntContent]='\n';
    m_Buf[nCntTime+nCntContent+1]='\0';

    LogStr=m_Buf;

    m_MutexLock.unlock();

    va_end(ValList);
    //同步写，异步插入队列

    if(true==m_IsAsync)
    {
        while(false==m_LogQueue->Push(LogStr));
    }
    else
    {
        m_MutexLock.lock();
        fputs(LogStr.c_str(),m_FP);
        m_MutexLock.unlock();
    }
}

Log::~Log()
{
    m_WriteStop=true;
    flush();
    if(nullptr!=m_FP)
    {
        fclose(m_FP);
    }

    if(nullptr!=m_Buf)
    {
        delete[] m_Buf;
    }

    if(nullptr!=m_LogQueue)
    {
        delete m_LogQueue;
    }
}

void Log::AsyncWriteLog()
{
    std::string LogString;
    while(!m_WriteStop)
    {
        if(m_LogQueue->Pop(LogString))
        {
            m_MutexLock.lock();
            fputs(LogString.c_str(),m_FP);
            m_MutexLock.unlock();
        }
    }
}