#include "http_conn.h"

const char* ok_200_title="OK";
const char* error_400_title="Bad Request";
const char* error_400_form="Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title="Forbidden";
const char* error_403_form="You do not have permission to get file from this server.\n";
const char* error_404_title="Not Found";
const char* error_404_form="The requested file was not found on this server.\n";
const char* error_500_title="Internal Error";
const char* error_500_form="There was an unusual problem serving the requested file.\n";
const char* doc_root="/var/www/html";

int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFD);
    int new_opt=old_opt|O_NONBLOCK;
    fcntl(fd,F_SETFD,new_opt);
    return old_opt;
}

void addfd(int epollfd,int fd,bool one_shot){
    epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLRDHUP|EPOLLET;
    if(one_shot)
        ev.events|=EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnonblocking(fd);
}

void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,nullptr);
    close(fd);
}

void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd=fd;
    event.events=ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;

void http_conn::close_conn(bool real_close){
    if(real_close&&m_sockfd!=-1){
        init();
        removefd(m_epollfd,m_sockfd);
        m_sockfd=-1;
        --m_user_count;
    }
}

void http_conn::init(int sockfd,const sockaddr_in& addr){
    m_sockfd=sockfd;
    m_address=addr;

    //调试用，关闭time_wait
    //int reuse=1;
    //setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    addfd(m_epollfd,m_sockfd,true);
    ++m_user_count;

    init();
}

void http_conn::init(){
    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    m_read_idx=0;
    m_checked_idx=0;
    m_start_line=0;

    memset(m_write_buf,'\0',WRITE_BUFFER_SIZE);
    m_write_idx=0;

    m_check_state=CHECK_STATE_REQUESTLINE;
    m_method=GET;

    memset(m_real_file,'\0',FILENAME_LEN);
    m_url=nullptr;
    m_version=nullptr;
    m_host=nullptr;
    m_content_len=0;
    m_linger=false;

    m_file_address=nullptr;
    __bzero(&m_file_stat,sizeof(m_file_stat));
    __bzero(&m_iv,sizeof(m_iv));
    m_iv_count=0;
}

http_conn::LINE_STATUS http_conn::parse_line(){
    while(m_checked_idx<m_read_idx){
        if(m_checked_idx>1&&m_read_buf[m_checked_idx]=='\n'){
            if(m_read_buf[m_checked_idx-1]=='\r'){
                m_read_buf[m_checked_idx-1]='\0';
                m_read_buf[m_checked_idx++]='\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        }
        else if(m_read_buf[m_checked_idx]=='\r'){
            if(m_checked_idx+1==m_read_idx)
                return LINE_OPEN;
            else if(m_read_buf[m_checked_idx+1]=='\n'){
                m_read_buf[m_checked_idx++]='\0';
                m_read_buf[m_checked_idx++]='\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        }
        ++m_checked_idx;
    }
    return LINE_OPEN;
}

bool http_conn::read(){
    if(m_read_idx>=READ_BUFFER_SIZE)
        return false;
    while(true){
        int ret=recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
        if(ret<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }
            return false;
        }
        else if(ret==0){
            return false;
        }
        else
            m_read_idx+=ret;
    }
    return true;
}

http_conn::HTTP_CODE http_conn::parse_request_line(char* text){
    char* method=text+strspn(text," \t");
    char* url=strpbrk(method," \t");
    if(url==nullptr)
        return BAD_REQUEST;
    *url++='\0';
    url+=strspn(url," \t");
    char* version=strpbrk(url," \t");
    if(version==nullptr)
        return BAD_REQUEST;
    *version++='\0';
    version+=strspn(version," \t");
    
    if(strcasecmp(method,"GET")==0)
        m_method=GET;
    else
        return BAD_REQUEST;
    
    if(strncasecmp(url,"http://",7)==0){
        url+=7;
        m_url=strchr(url,'/');
        if(m_url==nullptr||m_url[0]!='/')
            return BAD_REQUEST;
    }

    if(strcasecmp(version,"HTTP/1.1")==0)
        m_version="HTTP/1.1";
    else
        return BAD_REQUEST;
    
    m_check_state=CHECK_STATE_HEADER;
    return NO_REQUEST;    
}

http_conn::HTTP_CODE http_conn::parse_header(char* text){
    if(text[0]='\0'){
        if(m_content_len!=0){
            m_check_state=CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text,"Connection:",11)==0){
        text+=11;
        text+=strspn(text," \t");
        
    }
}

http_conn::HTTP_CODE http_conn::parse_content(char* text){

}

http_conn::HTTP_CODE http_conn::process_read(){

}

http_conn::HTTP_CODE http_conn::do_request(){

}

void http_conn::unmap(){

}

bool http_conn::write(){

}

bool http_conn::add_response(const char* format,...){

}

bool http_conn::add_status_line(int status,const char* title){

}

bool http_conn::add_header(int content_len){

}

bool http_conn::add_content_len(int content_len){

}

bool http_conn::add_linger(){

}

bool http_conn::add_blank_line(){

}

bool http_conn::add_content(const char* content){

}

bool http_conn::process_write(HTTP_CODE ret){

}

void http_conn::process(){

}
