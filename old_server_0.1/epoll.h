/*
 * @Author: your name
 * @Date: 2021-07-26 23:45:49
 * @LastEditTime: 2021-07-26 23:49:15
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \WebServer-masterf:\MyWebServer\old_server_0.1\epoll.h
 */
#ifndef EVENPOLL
#define EVENPOLL
#include "requestData.h"

const int MAXEVENTS = 5000;
const int LINSTEQ   = 1024;

int epoll_init();
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events);
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events);
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events);
int my_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout);


#endif