#include "http_conn.h"

const char* ok_200_title="OK";
const char* ok_string="<html><body></body></html>\n";
const char* error_400_title="Bad Request";
const char* error_400_form="Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title="Forbidden";
const char* error_403_form="You do not have permission to get file from this server.\n";
const char* error_404_title="Not Found";
const char* error_404_form="The requested file was not found on this server.\n";
const char* error_500_title="Internal Error";
const char* error_500_form="There was an unusual problem serving the requested file.\n";
const char* doc_root="/home/jiangzhiwei/mywebroot";

myepoll http_conn::m_epoll=myepoll();
int http_conn::m_user_count=0;

void http_conn::close_conn(bool real_close){
    if(real_close&&m_sockfd!=-1){
        m_epoll.DelFd(m_sockfd);
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

    m_epoll.AddFd(m_sockfd,true);
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
    if(m_read_idx>=READ_BUFFER_SIZE){
        printf("rdbuffer busy\n");
        return false;
    }
    while(true){
        int readlen=recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
        if(readlen<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }
            return false;
        }
        else if(readlen==0){
            return false;
        }
        else
            m_read_idx+=readlen;
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
    char* last=strpbrk(version," \t");
    if(last!=nullptr)
        *last='\0';
    
    if(strcasecmp(method,"GET")==0)
        m_method=GET;
    else
        return BAD_REQUEST;
    
    m_url=url;
    if(strncasecmp(m_url,"http://",7)==0){
        m_url+=7;
        m_url=strchr(m_url,'/');
    }

    if(m_url==nullptr||m_url[0]!='/')
        return BAD_REQUEST;

    if(strcasecmp(version,"HTTP/1.1")==0)
        m_version=version;
    else
        return BAD_REQUEST;
    
    m_check_state=CHECK_STATE_HEADER;
    return NO_REQUEST;    
}

http_conn::HTTP_CODE http_conn::parse_header(char* text){
    if(text[0]=='\0'){
        if(m_content_len!=0){
            m_check_state=CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    text+=strspn(text," \t");
    if(strncasecmp(text,"Connection:",11)==0){
        text+=11;
        text+=strspn(text," \t");
        if(strcasecmp(text,"keep-alive")==0)
            m_linger=true;
    }
    else if(strncasecmp(text,"Content-Length:",15)==0){
        text+=15;
        text+=strspn(text," \t");
        if((m_content_len=atoi(text))==0){
            return BAD_REQUEST;
        }
    }
    else if(strncasecmp(text,"Host:",5)==0){
        text+=5;
        text+=strspn(text," \t");
        m_host=text;
        if(m_host==nullptr||m_host[0]=='\0')
            return BAD_REQUEST;
    }
    else{
        printf("Unknown header: %s\n",text);
        return BAD_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char* text){
    //只确认content是否完整，并不做具体分析与响应
    if(m_read_idx>=(m_start_line+m_content_len)){
        text[m_content_len]='\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATUS cur_line_status=LINE_OK;
    HTTP_CODE ret=NO_REQUEST;
    char* text=nullptr;

    while(((m_check_state==CHECK_STATE_CONTENT)&&(cur_line_status==LINE_OK))||((cur_line_status=parse_line())==LINE_OK)){
        text=get_line();
        m_start_line=m_checked_idx;
        printf("Get one http line:%s\n",text);

        switch(m_check_state){
            case(CHECK_STATE_REQUESTLINE):{
                ret=parse_request_line(text);
                if(ret==BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case(CHECK_STATE_HEADER):{
                ret=parse_header(text);
                if(ret==GET_REQUEST)
                    return do_request();
                else if(ret==BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case(CHECK_STATE_CONTENT):{
                //与游双老师书上实现不同
                ret=parse_content(text);
                if(ret==GET_REQUEST)
                    return do_request();
                else
                    return NO_REQUEST;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request(){
    strncpy(m_real_file,doc_root,FILENAME_LEN-1);
    int len=strlen(doc_root);
    strncat(m_real_file,m_url,FILENAME_LEN-1-len);

    if(stat(m_real_file,&m_file_stat)==-1)
        return NO_RESOURCE;
    if(!(m_file_stat.st_mode&S_IROTH))
        return FORBIDDEN_REQUEST;
    if(S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    int filefd=open(m_real_file,O_RDONLY);
    m_file_address=(char*)mmap(nullptr,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,filefd,0);
    if(m_file_address==MAP_FAILED){
        printf("map failed\n");
        m_file_address=nullptr;
        close(filefd);
        return INTERNAL_ERROR;
    }
    close(filefd);
    return FILE_REQUEST;
}

void http_conn::unmap(){
    if(m_file_address!=nullptr){
        munmap(m_file_address,m_file_stat.st_size);
        m_file_address=nullptr;
    }
}

bool http_conn::write(){
    int writelen=0;
    int bytes_to_send=m_iv[0].iov_len+m_iv[1].iov_len;

    if(bytes_to_send==0){
        m_epoll.ModFd(m_sockfd,EPOLLIN,true);
        init();
        return true;
    }

    writelen=writev(m_sockfd,m_iv,m_iv_count);
    if(writelen<0){
        if(errno==EAGAIN){
            m_epoll.ModFd(m_sockfd,EPOLLOUT,true);
            return true;
        }
        unmap();
        return false;
    }
    unmap();
    if(writelen==bytes_to_send){
        if(m_linger){
            init();
            m_epoll.ModFd(m_sockfd,EPOLLIN,true);
            return true;
        }
        return false;
    }
    else{
        printf("TCP wbuffer busy\n");
        return false;
    }
}

bool http_conn::add_response(const char* format,...){
    if(m_write_idx>=WRITE_BUFFER_SIZE){
        printf("http wbuffer busy\n");
        return false;
    }
    
    va_list arg_list;
    va_start(arg_list,format); 
    int writelen=vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,arg_list);
    va_end(arg_list);

    if(writelen<0){
        printf("vsnprintf error\n");
        return false;
    }
    else if(writelen+m_write_idx>WRITE_BUFFER_SIZE-1){
        printf("vsnprintf wbuffer busy\n");
        return false;
    }
    m_write_idx+=writelen;
    return true;
}

bool http_conn::add_status_line(int status,const char* title){
    return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}

bool http_conn::add_header(int content_len){
    return add_content_len(content_len)&&add_linger()&&add_blank_line();
}

bool http_conn::add_content_len(int content_len){
    return add_response("Content-Length: %d\r\n",content_len);
}

bool http_conn::add_linger(){
    return add_response("Connection: %s\r\n",m_linger?"keep-alive":"close");
}

bool http_conn::add_blank_line(){
    return add_response("%s","\r\n");
}

bool http_conn::add_content(const char* content){
    return add_response("%s",content);
}

bool http_conn::process_write(HTTP_CODE ret){
    switch(ret){
        case(BAD_REQUEST):{
            if(!(add_status_line(400,error_400_title)&&add_header(strlen(error_400_form))&&add_content(error_400_form)))
                return false;
            break;
        }
        case(INTERNAL_ERROR):{
            if(!(add_status_line(500,error_500_title)&&add_header(strlen(error_500_form))&&add_content(error_500_form)))
                return false;
            break;
        }
        case(FORBIDDEN_REQUEST):{
            if(!(add_status_line(403,error_403_title)&&add_header(strlen(error_403_form))&&add_content(error_403_form)))
                return false;
            break;
        }
        case(NO_RESOURCE):{
            if(!(add_status_line(404,error_404_title)&&add_header(strlen(error_404_form))&&add_content(error_404_form)))
                return false;
            break;
        }
        case(FILE_REQUEST):{
            if(!add_status_line(200,ok_200_title))
                return false;
            if(m_file_stat.st_size>0){
                if(!add_header(m_file_stat.st_size))
                    return false;
                m_iv[0].iov_base=m_write_buf;
                m_iv[0].iov_len=m_write_idx;
                m_iv[1].iov_base=m_file_address;
                m_iv[1].iov_len=m_file_stat.st_size;
                m_iv_count=2;
                return true;
            }
            else{
                if(!(add_header(strlen(ok_string))&&add_content(ok_string)))
                    return false;
            }
            break;
        }
        default:
            return false;
    }
    m_iv[0].iov_base=m_write_buf;
    m_iv[0].iov_len=m_write_idx;
    m_iv_count=1;
    return true;
}

void http_conn::process(){
    HTTP_CODE ret=process_read();
    if(ret==NO_REQUEST){
        m_epoll.ModFd(m_sockfd,EPOLLIN,true);
        return;
    }
    bool write_ret=process_write(ret);
    if(!write_ret){
        timer->set_cbfunc(nullptr);
        close_conn();
        return;
    }
    m_epoll.ModFd(m_sockfd,EPOLLOUT,true);
}
