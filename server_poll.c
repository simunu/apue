/*********************************************************************************
 *      Copyright:  (C) 2022 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  server_poll.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/14/22)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "11/14/22 08:40:36"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <ctype.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 13

void print_usage(char *progname);
int server_start(char *host,int port);

int main(int argc,char **argv)
{
	int ch;
	int i = 0;
	int rv = -1;
	int port = 0;
	int found = 0;
	int max_fds = 0;
	int conn_fd = 0;
	int listen_fd = 0; 
	char buf[1024];
	struct pollfd fds[128];
	
	struct option opts[] = {
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
							
	while((ch = getopt_long(argc,argv,"p:h",opts,NULL)) != -1)
	{
		switch(ch)
		{
			case'p':
				port = atoi(optarg);
				break;
			case'h':
				print_usage(argv[0]);
				return 0;
			default:
				break;
		}
	}
	if(!port)
	{
		printf("please input the port\n");
		print_usage(argv[0]);
		return 0;
	}
	if((listen_fd = server_start(NULL,port)) < 0)
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d] successfully\n",listen_fd);

	
	for(i=0;i<128;i++) 
	{
		fds[i].fd = -1;
		fds[i].events = POLLIN; 
	}
	fds[0].fd = listen_fd;

	for( ; ; )
	{
		rv = poll(fds,max_fds+1,-1);
		if(rv < 0)
		{
			printf("select failure:%s\n",strerror(errno));
			return -1;
		}

		else if(rv == 0 )
		{
			printf("select get timeout\n");
			continue;
		}
		if(fds[0].revents&POLLIN)
		{
			if((conn_fd = accept(listen_fd,(struct sockaddr *)NULL,NULL)) < 0)
			{
				printf("accept new client failure:%s\n",strerror(errno));
				continue;
			}

			for(i=1;i<128;i++)
			{
				if(fds[i].fd < 0)
				{
					printf("accept new client[%d]\n",conn_fd);
					fds[i].fd = conn_fd;
					fds[i].events = POLLIN;
					max_fds = max_fds>i?max_fds:i;
					found = 1;
					break;
				}
			}
			if(!found)
			{
				printf("accept new client[%d] but full ,so refuse it\n",conn_fd);
				close(conn_fd);
				continue;
			}
		}
		for(i = 1; i<= max_fds;i++)
		{
			if(fds[i].revents&POLLIN)
			{
				rv = read(fds[i].fd,buf,sizeof(buf));
				if(rv <= 0)
				{
					printf("read failure or get disconnect\n");
					close(fds[i].fd);
					fds[i].fd = -1;
				}
				else
				{
					printf("socket[%d] read get %d bytes data:%s\n",fds[i].fd,strlen(buf),buf);
					if(write(fds[i].fd,buf,rv) < 0)
					{
						printf("socket[%d] write failure: %s\n",fds[i].fd,strerror(errno));
						close(fds[i].fd);
						fds[i].fd = -1;
					}
				}
			}
		}
	}
	close(listen_fd);
	return 0;																	
}

void print_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("-p(--port):sepcify server port.\n");
	printf("-h(--Help):print this help information.\n");
	return ;
}

int server_start(char *host,int port)
{
	int  on = 1;
	int  listen_fd = -1;
	socklen_t  len;	
	struct sockaddr_in servaddr;

	if((listen_fd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
	{
		printf("create socket failure:%s.\n",strerror(errno));
		return -1;
	}
	setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	if((bind(listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr))) < 0 )
	{
		printf("bind socket failure:%s\n",strerror(errno));
		return -1;
	}
	if(listen(listen_fd,BACKLOG) < 0 )
	{
		printf("listen failure:%s\n",strerror(errno));
		return -1;
	}
	return listen_fd;
}
