/*********************************************************************************
 *      Copyright:  (C) 2022 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  server_select.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/09/22)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "11/09/22 15:24:04"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 13
#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

void print_usage(char *progname);
void msleep(unsigned long ms);
int server_start(char *host,int port);

int main(int argc, char **argv)
{
	int i;
	int ch;
	int	rv;
	int port = 0;
	int found = 0;
	int	maxfd = 0;
	int listen_fd;
	int conn_fd;
	int fds_array[1024];
	char buf[1024];
	fd_set	rdset;

	struct option opts[]= {
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	while((ch=getopt_long(argc, argv, "p:h", opts, NULL)) != -1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				print_usage(argv[0]);
				return 0;
		}
	}
	if(!port)
	{
		printf("please input the port!\n");
		print_usage(argv[0]);
		return 0;
	}
	if((listen_fd = server_start(NULL,port)) < 0 )
	{
		printf("create socket failure: %s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d] successfully!\n",listen_fd);

	for(i=0;i<ARRAY_SIZE(fds_array);i++)
	{
		fds_array[i] = -1;
	}
	fds_array[0] = listen_fd;

	for( ; ; )
	{
		FD_ZERO(&rdset);
		for(i=0; i<ARRAY_SIZE(fds_array) ; i++)
		{
			if( fds_array[i] < 0 )
				continue;
			maxfd = fds_array[i]>maxfd ? fds_array[i] : maxfd;
			FD_SET(fds_array[i], &rdset);
		}

		if((select(maxfd+1,&rdset,NULL,NULL,NULL)) < 0 )
		{
			printf("select failure:%s\n",strerror(errno));
			break;
		}
		else if((select(maxfd+1,&rdset,NULL,NULL,NULL)) == 0 )
		{
			printf("select get timeout\n");
			continue;
		}

		if(FD_ISSET(listen_fd,&rdset))
		{
			if((conn_fd = accept(listen_fd,(struct sockaddr *)NULL,NULL)) < 0)
			{
				printf("accept failure:%s\n",strerror(errno));
				return -1;
			}
			for(i = 0;i<ARRAY_SIZE(fds_array);i++)
			{
				if(fds_array[i] < 0)
				{
					printf("accept new client[%d]\n",conn_fd);
					fds_array[i] = conn_fd;
					found = 1;
					break; 
				}
			}
			if(!found)
			{
				printf("accept new client[%d] but full,so accept failure\n",conn_fd);
				close(conn_fd);
			}
		}
		else
		{
			for(i=0;i<ARRAY_SIZE(fds_array);i++)
			{
				if(fds_array[i]<0 || !FD_ISSET(fds_array[i],&rdset))
					continue;
				if((rv=read(fds_array[i],buf,sizeof(buf))) <= 0)
				{
					printf("socket[%d] read failure or get disconnect\n",fds_array[i]);
					close(fds_array[i]);
					fds_array[i] = -1;
				}
				else
				{
					printf("socket[%d] read data:%s\n",fds_array[i],buf);
					if(write(fds_array[i],buf,rv) < 0 )
					{
						printf("socket[%d] write failure :%s\n",fds_array[i],strerror(errno));
						close(fds_array[i]);
						fds_array[i] = -1;
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

	if(bind(listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
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

void msleep(unsigned long ms)
{
	struct timeval tv;
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms%1000)*1000;
	select(0,NULL,NULL,NULL,&tv);
}
