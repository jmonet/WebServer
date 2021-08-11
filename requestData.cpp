/*
 * @Author: your name
 * @Date: 2021-08-04 23:12:23
 * @LastEditTime: 2021-08-11 23:50:41
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\requestData.cpp
 */
#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;

//test
#include <iostream>
using namespace std;


pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MimeType::lock = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::string, std::string> MimeType::mime;

priority_queue<mytimer* , deque<mytimer*>, timerCmp> myTimerQueue;

//空参数构造函数
requestData::requestData():
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start),
    keep_alive(false), againTimes(0), timer(NULL)
{
    cout << "requestData constructed !" << endl;
}

//有参构造函数
requestData::requestData(int epoll_fd, int _fd, std::string _path):
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start),
    keep_alive(false), againTimes(0), timer(NULL),
    path(_path), fd(_fd), epollfd(epoll_fd)
{

}

//析构函数
requestData::~requestData()
{
    cout << "~requestData()" << endl;
    struct epoll_event ev;
    // 超时的一定都是读请求，没有“被动”写
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void*)this;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd ,&ev);
    if(timer != NULL)
    {
        timer->clearReq();
        timer = NULL;
    } 
    close(fd);
}

void requestData::setFd(int _fd)
{
    fd = _fd;
}


void requestData::seperateTimer()
{
    if (timer)
    {
        timer->clearReq();
        timer = NULL;
    }
}

void requestData::getFd()
{
    return fd;
}

void requestData::addTimer(mytimer *mtimer)
{
    if (timer == NULL )
        timer = mtimer;
}

void requestData::setFd(int _fd)
{
    fd = _fd;
}

void requestData::reset()
{
    againTimes = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PRASE_URI;
    h_state = h_start;
    headers.clear();
    keep_alive = false;
}

void requestData::handleRequest()
{
    char buff[MAX_BUFF];
    bool isError = false;
    while (true)
    {
        int read_num = readn(fd, buff, MAX_BUFF);

        if( read_num < 0 )
        {
            perror("read_num < 0");
            isError = ture;
            break;
        }
        else if( read_num == 0 )
        {
            // 有请求出现但是出现读不到数据，可能是request Aborted，或者来自网络的原因数据没到达
            perror("read_num == 0");
            if( error == EAGAIN )   //错误EAGAIN，提示应用程序现在没有数据可读请稍后再试
            {
                if ( againTimes > AGAIN_MAX_TIMES )    //请求超过200次就放弃
                    isError = true;
                else
                    ++againTimes;
            }
            else if (error != 0)
                isError = true;
            break;
        }
        string now_read(buff, buff + read_num); //string s(str,stridx) //将字符串str内“始于位置stridx”的部分当作字符串的初值
        content += now_read;

        if( state == STATE_PARSE_URI )
        {
            int flag = this->parse_URI();
            if ( flag == PRASE_URI_AGAIN )
            {
                break;
            }
            else if (flag == PARSE_URI_ERROR )
            {
                perror("2");
                isError = true;
                break;
            }
        }

        if( state == STATE_PARSE_HEADERS )
        {
            int flag = this->parse_Headers();
            if ( flag == PARSE_HEADER_AGAIN )
            {
                break;
            }
            else if (flag == PARSE_HEADER_ERROR )
            {
                perror("3");
                isError = true;
                break;
            }
            if( method == METHOD_POST )
            {
                state = STATE_RECV_BODY;
            }
            else
            {
                state = STATE_ANALYSIS;
            }
        }

        if( state == STATE_RECV_BODY )
        {
            int content_length = -1;
            if (headers.find("Content-length") != headers.end())
            {
                content_length = stoi(headers["Content-length"]);
            }
            else
            {
                isError = true;
                break;
            }
            if( content.size() < content_length )
                continue;
            state = STATE_ANALYSIS;
        }

        if( state == STATE_ANALYSIS )
        {
            int flag = this->analysisRequesr();
            if( flag < 0 )
            {
                isError = true;
                break;
            }
            else if( flag == ANALYSIS_SUCCESS )
            {
                state = STATE_FINISH;
                break;
            }
            else
            {
                isError = true;
                break;
            }
        }
    }

    if (isError)    //错误处理，分别在上面readn， parse_URI， parse_Headers， headers.find， analysisRequesr中有判断
    {
        delete this;
        return;
    }
    //加入epoll继续
    if (state == STATE_FINISH )
    {
        if(keep_alive)
        {
            printf("ok\n");
            this->rest();
        }
        else
        {
            delete this;
            return;
        }
    }

    // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，然后分离失败后，又加入队列，最后被超时删除，然后正在线程中进行的任务出错，double free错误。
    // 利用锁新增时间信息
    pthread_mutex_lock(&qlock);
    myTimer *mtimer = new mytimer(this, 500);
    timer = mtimer;
    myTimerQueue.push(mtimer);
    pthread_mutex_unlock(&qlock);

    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    int ret = epoll_mod(epollfd, fd, static_cast<void*>(this), _epo_event);
    if( ret < 0)
    {
        //返回错误处理
        delete this;
        return;
    }
}


int requestData::parse_URI()
{
    string &str = content;
    // 读到完整的请求行再进行解析请求，如下：
    // POST /chapter17/index.html HTTP/1.1
    int pos = str.find('\r', now_read_pos);
    if( pos < 0 )
    {
        return PARSE_URI_ERROR;
    }
    // 去掉请求行所占的空间，释放掉以节省空间
    // requset_line 截取从0到pos位置的字符串
    string request_line = str.substr(0, pos);
    if( str.size() > pos + 1 )
        str = str.substr( pos + 1 );
    else
        str.clear();

    // 请求行中的方法 Method
    pos = request_line.find("GET");
    if( pos < 0)
    {
        pos = request_line.find("POST");
        if( pos < 0 )
            return PARSE_URI_ERROR;
        else
            method = METHOD_POST;
    }
    else
    {
        method = METHOD_GET;
    }

    // 请求行中的filename
    // 从request_line中的第pos个位置查找 '/'
    pos = request_line.find('/', pos);
    if( pos < 0 )
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        // 从request_line中的第pos个位置查找 空格
        int _pos = request_line.find(' ', pos);
        if( _pos - pos > 1 )
        {
            file_name = request_line.substr(pos+1, _pos - pos + 1);
            int __pos = file_name.find('?');
            if( __pos >= 0 )
            {
                file_name = file_name.substr(0, __pos);
            }
        }
        else
            file_name = "index.html";
    }
    pos = _pos;

    //请求行中的版本号
    pos = request_line.find("/", pos);
    if( pos < 0 )
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        if( request_line.size() - pos <= 3)
        {
            return PARSE_URI_ERROR;
        }
        else
        {
            string ver = request_line.substr(pos+1, 3);
            if( ver == "1.0" )
                HTTPversion = HTTP_10;
            else if( ver == "1.1" )
                HTTPversion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }

    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

//语义分析头
int requestData::parse_Headers()
{
    // 请求头例子如下：
    // Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/vnd.ms-powerpoint, 
    // application/msword, application/x-silverlight, application/x-shockwave-flash, */*  '\r''\n'
    string &str = content;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    for (int i = 0; i < str.size() && notFinish; i++)
    {
        switch(h_state)
        {
            case h_start:   //h_start 起始
            {
                if( str[i] == '\n' || str[i] == '\r' )
                    break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key:    //h_key 头部字段名
            {
                if( str[i] == ':' )
                {
                    key_end = i;
                    if( key_end - key_start <= 0 )
                        return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                }
                else if( str[i] == '\n' || str[i] == '\r' )
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_colon:   //h_colon 标题头冒号
            {
                if ( str[i] == ' ' )
                {
                    h_state = h_spaces_after_colon;
                }
                else   
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_spaces_after_colon:  //冒号后的空格
            {
                h_state = h_value;
                value_start = i;
                break;
            }
            case h_value:
            {
                if( str[i] == '\r' )
                {
                    h_state = h_CR;
                    value_end = i;
                    if ( value_end - value_start <= 0 )
                        return PARSE_HEADER_ERROR; 
                }
                else if( i - value_start > 255 )
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_CR:
            {
                if( str[i] == '\n' )
                {
                    h_state = h_LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_LF:
            {
                if( str[i] == '\r' )
                {
                    h_state = h_end_CR;
                }
                else
                {
                    key_start = i;
                    h_state = h_key;
                }
                break;
            }
            case h_end_CR:
            {
                if (str[i] == '\n')
                {
                    h_state = h_end_LF;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_end_LF:
            {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
        
    }
    if( h_state == h_end_LF )
    {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}


int requestData::analysisRequest()
{
    if ( method == METHOD_POST )
    {

    }
    else if( method == METHOD_GET )
    {

    }
    else
        return ANALYSIS_ERROR;
}

