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

int setnonblocking(inf fd){

}

void addfd(int epollfd,int fd,bool one_shot){

}

void removefd(int epollfd,int fd){

}

void modfd(int epollfd,int fd,int ev){

}

int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;

void http_conn::init(int sockfd,const sockaddr_in& addr){

}

void http_conn::init(){

}

http_conn::LINE_STATUS http_conn::parse_line(){

}

bool http_conn::read(){

}

http_conn::HTTP_CODE http_conn::parse_request_line(char* text){

}

http_conn::HTTP_CODE http_conn::parse_header(char* text){

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