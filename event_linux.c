//
//  event_linux.c
//  klib
//
//  Created by LiKai on 16/1/8.
//  Copyright ? 2016�� LiKai. All rights reserved.
//
#ifdef linux

#include "event.h"

status_t
event_pool_init(event_pool_t* event_pool, socket_t server_socket, socket_event_callback callback)
{
	
}

status_t  event_dispatch(event_pool_t* event_pool)
{

	for (; ; )
	{
		nfds = epoll_wait(epfd, events, 20, 500);
		for (i = 0; i < nfds; ++i)
		{
			if (events[i].data.fd == listenfd) //���µ�����
			{
				connfd = accept(listenfd, (sockaddr *)&clientaddr, &clilen); //accept�������
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev); //���µ�fd��ӵ�epoll�ļ���������
			}
			else if (events[i].events&EPOLLIN) //���յ����ݣ���socket
			{
				n = read(sockfd, line, MAXLINE)) < 0 //��
				ev.data.ptr = md; //mdΪ�Զ������ͣ��������
				ev.events = EPOLLOUT | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);//�޸ı�ʶ�����ȴ���һ��ѭ��ʱ�������ݣ��첽����ľ���
			}
			else if (events[i].events&EPOLLOUT) //�����ݴ����ͣ�дsocket
			{
				struct myepoll_data* md = (myepoll_data*)events[i].data.ptr; //ȡ����
				sockfd = md->fd;
				send(sockfd, md->ptr, strlen((char*)md->ptr), 0); //��������
				ev.data.fd = sockfd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); //�޸ı�ʶ�����ȴ���һ��ѭ��ʱ��������
			}
			else
			{
				//�����Ĵ���
			}
		}
	}
}



#endif