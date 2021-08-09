/*
 * @Author: your name
 * @Date: 2021-08-07 22:42:43
 * @LastEditTime: 2021-08-08 00:21:33
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\socket_bind_listen.cpp
 */
#include<errno.h>
#include<assert.h>
#include<string.h>
#include <sys/epoll.h>
#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>


int socket_bind_listen(int port)
{
    if( port < 1024 || port > 65535 )
    {
        perror("port num error");
        return -1;
    }

    int listen_fd = 0;
    if( listen_fd = socket(AF_INET, SOCK_STREAM,IPPORTO_TCP) < 0 )
    {
        perror("socket failed");
        return -1;
    }

    struct sockaddr server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if( bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
    {
        perror("bind failed");
        return -1;
    }

    if( listen(listen_fd, 1024 + 1) < 0 )
    {
        perror("listen failed");
        return -1;
    }

    return listen_fd;

}

int main(int argc, char* argv[])
{
    if(argv != 3)
    {
        usage(argv[0]);
        return 1;
    }
    int listen_sock = start_up(argv[1], atoi(argv[2]));
    
    int epoll_fd = 0;
    if( (epoll_fd = epoll_create(256)) < 0 )
    {
        perror("epoll create failed");
        return 2;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listen_sock;
    if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) < 0 )
    {
        perror("epoll_fd CTL ADD failed!");
        return 3;
    }
    int nums = 0;
    struct epoll_event *ready_events[64];
    int len = 64;
    int timeout = -1;


    while(true)
    {
        switch(nums = epoll_wait(epoll_fd, ready_events, len, timeout) )    //epoll_wait返回的是活跃用户数
        {
            case 0:
                printf("time out....");
                break;
            case -1:
                printf("epoll_wait");
                break;
            default:
                {
                    int i = 0;
                    for(; i < nums; i++)
                    {
                        int fd = ready_events[i].data.fd;
                        if(fd == listen_sock && ready_events[i].events & EPOLLIN)
                        {
                            struct sockaddr_in client;
                            socklen_t len = sizeof(client);
                            int new_fd = accept(listen_sock, (struct sockaddr*)&client, &len);
                            if(new_fd < 0)
                            {
                                perror("accept failed");
                                continue;
                            }

                            printf("get a new accept client:%s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));

                            event.events = EPOLLIN || EPOLLET;  //重写一遍event的数据
                            event.data.fd = new_fd;
                            set_nonblock(new_fd);
                            epoll_ctl(accept, EPOLL_CTL_ADD, new_fd, &event);
                        }
                        else
                        {
                            if(ready_events[i].events & EPOLLIN)
                            {
                                char buf[1024];
                                ssize_t s = read(fd, buf, sizeof(buf)-1);
                                if( s > 0 )
                                {
                                    buf[s] = 0;
                                    printf("client#%s\n",buf);
                                    event.events = EPOLLOUT | EPOLLET;
                                    event.data.fd = fd;
                                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
                                }
                                else if( s == 0 )
                                {
                                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                                    close(fd);
                                    printf("client close...");
                                }
                                else
                                {
                                    perror("read failed");
                                }
                            }
                            else if(ready_events[i].events & EPOLLOUT)
                            {
                                char buf[1024];
                                sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n<html><h2>hello</h2></html>");
                                write(fd, buf, strlen(buf) );
                                epoll_ctl(epoll_fd , EPOLL_CTL_DEL, fd, NULL);
                                close(fd);
                            }
                            else
                            {}
                        }
                    }
                }
                break;
        }
    }
    return 0;
}
