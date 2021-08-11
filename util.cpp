/*
 * @Author: your name
 * @Date: 2021-07-27 23:22:00
 * @LastEditTime: 2021-08-10 00:00:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\util.cpp
 */
#include "util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    //'\0'的ascii码值是0
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return ;
}

// 设置文件描述符 fd 为非阻塞模式
int setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if( flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if( fcntl(fd, F_SETFL, flag) == -1)
        return -1;

    return 0;
}

ssize_t readn(int fd, void *buff, size_t n)
{
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char *ptr = (char*)buff;
    while( nleft > 0 )
    {
        if( (nread = read(fd,ptr,nleft)) < 0 )  //对读到小于0的数据进行三个错误判断,如果成功读取的话,返回读到的字节数大小,否则返回-1
        {
            if( errno == EINTR )    //读的过程中遇到了中断
                nread = 0;
            else if ( errno == EAGAIN ) //没有数据可读，请稍后再尝试
            {
                return readSum;
            }
            else
            {
                return -1;
            }
        }
        else if( nread == 0 )
            break;

        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}