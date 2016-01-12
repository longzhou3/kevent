//
//  event_win.c
//  klib
//
//  Created by LiKai on 16/1/8.
//  Copyright ? 2016�� LiKai. All rights reserved.
//
#ifndef linux

#include "event.h"


#include "socket.h"

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF databuff;
	char buffer[2014];
	int BufferLen;
	int operationType;
}PER_IO_OPERATEION_DATA, *LPPER_IO_OPERATION_DATA, *LPPER_IO_DATA, PER_IO_DATA;

/**
* �ṹ�����ƣ�PER_HANDLE_DATA
* �ṹ��洢����¼�����׽��ֵ����ݣ��������׽��ֵı������׽��ֵĶ�Ӧ�Ŀͻ��˵ĵ�ַ��
* �ṹ�����ã��������������Ͽͻ���ʱ����Ϣ�洢���ýṹ���У�֪���ͻ��˵ĵ�ַ�Ա��ڻطá�
**/
typedef struct
{
	SOCKET socket;
	SOCKADDR_STORAGE ClientAddr;
	event_pool_t*    pool;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

static DWORD WINAPI ServerWorkThread(LPVOID IpParam)
{
	HANDLE CompletionPort = (HANDLE)IpParam;
	DWORD BytesTransferred;
	LPOVERLAPPED IpOverlapped;
	LPPER_HANDLE_DATA PerHandleData = NULL;
	LPPER_IO_DATA PerIoData = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;
	BOOL bRet = K_FALSE;

	while (K_TRUE) {
		bRet = GetQueuedCompletionStatus(CompletionPort, &BytesTransferred, (PULONG_PTR)&PerHandleData, (LPOVERLAPPED*)&IpOverlapped, INFINITE);
		if (bRet == 0) {
			printf("GetQueuedCompletionStatus Error->%d\n", GetLastError());
			return -1;
		}
		PerIoData = (LPPER_IO_DATA)CONTAINING_RECORD(IpOverlapped, PER_IO_DATA, overlapped);

		// ������׽������Ƿ��д�����
		if (0 == BytesTransferred) {
			closesocket(PerHandleData->socket);
			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}

		// ��ʼ���ݴ����������Կͻ��˵�����
		printf("A Client says->%s\n", PerIoData->databuff.buf);

		socket_event_t data;
		data.client_socket = PerHandleData->socket;
		data.data_len = PerIoData->databuff.len;
		data.data = PerIoData->databuff.buf;

		if(PerHandleData->pool->callback != K_NULL)
		{
			PerHandleData->pool->callback(&data);
		}

		// Ϊ��һ���ص����ý�����I/O��������
		ZeroMemory(&(PerIoData->overlapped), sizeof(OVERLAPPED)); // ����ڴ�
		PerIoData->databuff.len = 1024;
		PerIoData->databuff.buf = PerIoData->buffer;
		PerIoData->operationType = 0;  // read
		WSARecv(PerHandleData->socket, &(PerIoData->databuff), 1, &RecvBytes, &Flags, &(PerIoData->overlapped), NULL);
	}

}

status_t
event_pool_init(event_pool_t* event_pool, socket_t server_socket, socket_event_callback callback)
{
	HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == completionPort) {    // ����IO�ں˶���ʧ��  
		return K_ERROR;
	}
	event_pool->completionPort = completionPort;
	event_pool->server_socket = server_socket;
	event_pool->callback = callback;

	return K_SUCCESS;
}

status_t  event_push()
{
	
}

status_t  event_dispatch(event_pool_t* event_pool)
{

	// ����IOCP�߳�--�߳����洴���̳߳�  
	SYSTEM_INFO mySysInfo;
	GetSystemInfo(&mySysInfo);

	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		HANDLE ThreadHandle = CreateThread(NULL, 0, ServerWorkThread, event_pool->completionPort, 0, NULL);
		if (NULL == ThreadHandle) {
			return K_ERROR;
		}
		CloseHandle(ThreadHandle);
	}

	while (K_TRUE) {
		PER_HANDLE_DATA * PerHandleData = NULL;
		SOCKADDR_IN saRemote;
		int RemoteLen;
		SOCKET acceptSocket;

		// �������ӣ���������ɶˣ����������AcceptEx()  
		RemoteLen = sizeof(saRemote);
		acceptSocket = accept(event_pool->server_socket, (SOCKADDR*)&saRemote, &RemoteLen);
		if (SOCKET_ERROR == acceptSocket) {   // ���տͻ���ʧ��  
			printf("acceptSocket Error->%d\n", GetLastError());
			return -1;
		}

		// �����������׽��ֹ����ĵ����������Ϣ�ṹ  
		PerHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));  // �ڶ���Ϊ���PerHandleData����ָ����С���ڴ�  
		PerHandleData->socket = acceptSocket;
		PerHandleData->pool = event_pool;
		memcpy(&PerHandleData->ClientAddr, &saRemote, RemoteLen);
		//clientGroup.push_back(PerHandleData);       // �������ͻ�������ָ��ŵ��ͻ�������  

													// �������׽��ֺ���ɶ˿ڹ���  
		CreateIoCompletionPort((HANDLE)(PerHandleData->socket), event_pool->completionPort, (DWORD)PerHandleData, 0);


		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����  
		// ���½����׽�����Ͷ��һ�������첽  
		// WSARecv��WSASend������ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����      
		// ��I/O��������(I/O�ص�)  
		LPPER_IO_OPERATION_DATA PerIoData = NULL;
		PerIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATEION_DATA));
		ZeroMemory(&(PerIoData->overlapped), sizeof(OVERLAPPED));
		PerIoData->databuff.len = 1024;
		PerIoData->databuff.buf = PerIoData->buffer;
		PerIoData->operationType = 0;    // read  

		DWORD RecvBytes;
		DWORD Flags = 0;
		WSARecv(PerHandleData->socket, &(PerIoData->databuff), 1, &RecvBytes, &Flags, &(PerIoData->overlapped), NULL);
	}
}

#endif