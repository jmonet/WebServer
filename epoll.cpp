/*
 * @Author: your name
 * @Date: 2021-07-26 23:49:42
 * @LastEditTime: 2021-07-27 23:17:28
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\epoll.cpp
 */
#include "epoll.h"
#include <sys/epoll.h>
#include <errno.h>
#include "threadpool.h"

struct epoll_struct * events;

// 初始化描述符
int epoll_init()
{
    int epoll_fd = epoll_create(LISTENQ + 1);
    if(epoll_fd < 1)
        return -1;
    
    events = new epoll_create[MAXEVENTS];
    return epoll_fd;
}

// 注册描述符
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;

    //EPOLL_CTL_ADD：在文件描述符epfd所引用的epoll实例上注册目标文件描述符fd，并将事件事件与内部文件链接到fd
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_add error");
        return -1;
    }
    return 0;
}


//修改描述符状态
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.prt = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        perror("epoll_mod error");
        return -1;
    }
    return 0;
}

// 从epoll中删除描述符
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.prt = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll_mod error");
        return -1;
    }
    return 0;
}

// 返回活跃事件数
int my_epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout)
{
    int ret_count = epoll_wait(epfd, events, max_events, timeout);
    if (ret_count < 0)
    {
        perror("epoll wait error");
    }
    return ret_count;
}