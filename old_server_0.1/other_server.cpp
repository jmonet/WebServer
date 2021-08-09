/*
 * @Author: your name
 * @Date: 2021-08-07 16:25:27
 * @LastEditTime: 2021-08-07 16:27:16
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\other_server.cpp
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
static void usage(const char* proc)
{
	assert(proc);
	printf("usage: %s [ip] [port]\n",proc);
}
 
static int set_nonblock(int fd)
{
	int fl = fcntl(fd,F_SETFL);
	fcntl(fd,F_SETFL,fl|O_NONBLOCK);
}
 
// int my_read(int fd,char* buf,int len)
// {
// 	assert(buf);
// 	ssize_t total = 0;
// 	ssize_t s = 0;
// 	while((s = read(fd,&buf[total],len - 1 - total)) > 0&&errno != EAGAIN)
// 	{
// 		total += s;
// 	}
 
// 	return total;
// }
 
int start_up(char* ip,int port)
{
	assert(ip);
	assert(port > 0);
 
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
	{
		perror("socket");
		exit(1);
	}
	
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);
 
	if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0)
	{
		perror("bind");
		exit(2);
	}
 
	if(listen(sock,5) < 0)
	{
		perror("listen");
		exit(3);
	}
 
	return sock;
}
 
int main(int argc,char* argv[])
{
	if(argc != 3)
	{
		usage(argv[0]);
		return 1;
	}
	int listen_sock = start_up(argv[1],atoi(argv[2]));
	int epfd = epoll_create(256);
	if(epfd < 0)
	{
		perror("epoll_create");
		return 2;
	}
	
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = listen_sock;
	epoll_ctl(epfd,EPOLL_CTL_ADD,listen_sock,&ev);
	int nums = 0;
 
	struct epoll_event *ready_events[64];
	int len = 64;
	int timeout = -1;
 
	while(1)
	{
		switch(nums = epoll_wait(epfd,ready_events,len,timeout))
		{
			case 0:
				printf("timeout..");
				break;
			case -1:
				perror("epoll_wait");
				break;
			default:
				{
					int i = 0;
					for(;i < nums; i++)
					{
						int fd = ready_events[i].data.fd;
						if(fd == listen_sock && ready_events[i].events & EPOLLIN)
						{
							struct sockaddr_in client;
							socklen_t len = sizeof(client);
							int new_fd = accept(listen_sock,(struct sockaddr*)&client,&len);
							if(new_fd < 0)
							{
								perror("accept");
								continue;
							}
 
							printf("get a new client:%s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
 
							ev.events = EPOLLIN | EPOLLET;
							ev.data.fd = new_fd;
							set_nonblock(new_fd);
							epoll_ctl(epfd,EPOLL_CTL_ADD,new_fd,&ev);
						}
						else
						{
							if(ready_events[i].events & EPOLLIN)
							{
								char buf[1024];
								ssize_t s = read(fd,buf,sizeof(buf) - 1);
								if(s > 0)
								{
									buf[s] = 0;
									printf("client#%s\n",buf);
									ev.events = EPOLLOUT|EPOLLET;
									ev.data.fd = fd;
									epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
								}
								else if(s == 0)
								{
									epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
									close(fd);
									printf("client close...");
								}
								else
								{
									perror("read");
								}
							}
							else if(ready_events[i].events & EPOLLOUT)
							{
								char buf[1024];
								sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n<html><h2>hello</h2></html>");
								write(fd,buf,strlen(buf));
								epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
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

