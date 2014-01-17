#ifndef _PROXY_H
#define _PROXY_H

#include "MemPool.h"
#include <WinSock2.h>
#include "windows.h"
#include <iostream>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

#define MAXBUFFERSIZE      20*1024      //��������С
#define HTTP  "http://"
#define HTTPS  "CONNECT "

//Proxy �˿�
UINT pport = 8888;
DWORD WINAPI ProxyToServer(LPVOID pParam);
DWORD WINAPI UserToProxy  (LPVOID pParam);

//extern void //Log(CString info);

struct ProxySockets
{
    SOCKET  ProxyToUserSocket;	//���ػ�����PROXY �������socket
	SOCKET  ProxyToServSocket;	//PROXY �������Զ��������socket
	BOOL    IsProxyToUserClosed; // ���ػ�����PROXY �����״̬
	BOOL    IsProxyToServClosed; // PROXY �������Զ������״̬
};

struct ProxyParam
{
	char Address[256];		// Զ��������ַ
	HANDLE IsConnectedOK;	// PROXY �������Զ������������״̬
	HANDLE IsExit;	// PROXY �������Զ������������״̬
	ProxySockets * pPair;	// ά��һ��SOCKET��ָ��
	int Port;				// ��������Զ�������Ķ˿�
}; //�ṹ����PROXY SERVER��Զ����������Ϣ����

SOCKET listentsocket; //����������SOCKET

int StartProxyServer() //��������
{
	WSADATA wsaData;
	sockaddr_in local;
//	SOCKET listentsocket;

	//Log("�������������");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
	      //Log("\nError in Startup session.\n");
		  WSACleanup();
	      return -1;
	}

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(pport);
	//
	listentsocket = socket(AF_INET,SOCK_STREAM,0);
	//Log("���������׽���");

	if(listentsocket == INVALID_SOCKET)
	{
		//Log("\nError in New a Socket.");
		WSACleanup();
		return -2;
	}

	//Log("�������׽���");
	if(::bind(listentsocket,(sockaddr *)&local,sizeof(local))!=0)
	{
		//Log("\n Error in Binding socket.");
		WSACleanup();
		return -3;	
	}

	//Log("��������");
	if(::listen(listentsocket,100)!=0)
	{
		//Log("\n Error in Listen.");
		WSACleanup(); 
		return -4;
	}
//	::listentsocket=listentsocket; 

	//Log("���������߳�");
	HANDLE threadhandle = CreateThread(NULL, 0, UserToProxy, (LPVOID)NULL, NULL, NULL);

	return 1;
}

int CloseServer() //�رշ���
{
	closesocket(listentsocket);
//	WSACleanup();
	return 1;
}

//�������յ����ַ����õ�Զ��������ַ
int ResolveInformation( char * str, char *address, int * port)
{
//	char buff[MAXBUFFERSIZE], command[512], proto[128], *p;
	char *buf = (char*)MallocMem(MAXBUFFERSIZE);
	char *command = (char*)MallocMem(512);
	char *proto   = (char*)MallocMem(128);
	char *p = NULL;

	//CString strLog;

	sscanf(str,"%s%s%s",command,buf,proto);

	//strLog.Format("COMMAND : %s",command);
	//Log(strLog);
	//strLog.Format("BUFFER  : %s",buf);
	//Log(strLog);
	//strLog.Format("PROTOCL : %s",proto);
	//Log(strLog);

	p=strstr(buf,HTTP);

	//HTTP
	if(p)
	{
		UINT i;
		p+=strlen(HTTP);
		int j=0;
		for(i=0; i<strlen(p); i++)
		{
			if(*(p+i)==':') j=i;
			if( *(p+i)=='/') break;
		}

		*(p+i)=0;
		strcpy(address,p);
		if(j!=0){
			*port=atoi(p+j);
		}
		else{
			*port=80;	//ȱʡ�� http �˿�
		}
		p = strstr(str,HTTP);

		//ȥ��Զ��������: 
		//GET http://www.njust.edu.cn/ HTTP1.1  == > GET / HTTP1.1
		for(UINT j=0;j< i+strlen(HTTP);j++)
		{
			*(p+j)=' ';
		}

		//strLog.Format("ADDR : %s",address);
		//Log(strLog);
		//strLog.Format("PORT : %d",*port);
		//Log(strLog);

		return 1;
	}
	//sscanf(str,"%s%s%s",command,buf,proto);
	strcpy(buf,str);
	p=strstr(buf,HTTPS);
	if(p)
	{
		UINT i;
		p+=strlen(HTTPS);
		int j=0;
		for(i=0; i<strlen(p); i++)
		{
			if(*(p+i)==':') j=i;
			if( *(p+i)==' ') break;
		}
		*(p+i)=0;
		strcpy(address,p);
		if(j!=0){
			*port=atoi(p+j+1);
			*(address+j)=0;
		}
		else{
			*port=443;	//ȱʡ�� http �˿�
		}
		p = strstr(str,address);

		//ȥ��Զ��������: 
		//GET http://www.njust.edu.cn/ HTTP1.1  == > GET / HTTP1.1
		/*for(UINT j=0;j< i;j++)
		{
			*(p+j)=' ';
		}*/
		
		//*port=443;	//ȱʡ�� http �˿�

		//strLog.Format("ADDR : %s",address);
		//Log(strLog);
		//strLog.Format("PORT : %d",*port);
		//Log(strLog);*/

		return 1;
	}

	return 0; 
}

// ȡ�����ص����ݣ�����Զ������
DWORD WINAPI UserToProxy(void *pParam)
{
	sockaddr_in from;
	int  fromlen = sizeof(from);

	char Buffer[MAXBUFFERSIZE];
	int  Len;

	SOCKET ProxyToUserSocket;
	ProxySockets SPair;
	ProxyParam   ProxyP;

	HANDLE pChildThread;
	int retval;

	//Log("���������û�����");
	ProxyToUserSocket = accept(listentsocket,(struct sockaddr*)&from,&fromlen);
	//Log("��������");

	//Log("�����µ������߳�");
	HANDLE threadhandle = CreateThread(NULL, 0, UserToProxy, (LPVOID)pParam, NULL, NULL);
	CloseHandle(threadhandle);
	if( ProxyToUserSocket==INVALID_SOCKET)
	{ 
		//Log( "\nError  in accept "); 
		return -5;
	}

	//���ͻ��ĵ�һ������
	SPair.IsProxyToUserClosed = FALSE;
	SPair.IsProxyToServClosed = TRUE ;
	SPair.ProxyToUserSocket = ProxyToUserSocket;

	//Log("�����û�����");
	retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);
	 
	if(retval==SOCKET_ERROR)
	{ 
		//Log("\nError Recv"); 
		if(SPair.IsProxyToUserClosed == FALSE)
		{
			closesocket(SPair.ProxyToUserSocket);
			SPair.IsProxyToUserClosed = TRUE;

			return 0;
		}
	}

	if(retval==0)
	{
		//Log("Client Close connection\n");
		if(SPair.IsProxyToUserClosed==FALSE)
		{	
			closesocket(SPair.ProxyToUserSocket);
			SPair.IsProxyToUserClosed=TRUE;

			return 0;
		}
	}
	Len = retval;

	//
	SPair.IsProxyToUserClosed = FALSE;
	SPair.IsProxyToServClosed = TRUE;
	SPair.ProxyToUserSocket = ProxyToUserSocket;
	ProxyP.pPair=&SPair;
	ProxyP.IsConnectedOK = CreateEvent(NULL,TRUE,FALSE,NULL);
	struct hostent *hp;
	hp = gethostbyname("proxy.tencent.com");


	//Log("�����û�����");
	if(hp==NULL){
		ResolveInformation( Buffer,ProxyP.Address,&ProxyP.Port);
	}
	else{
		strcpy(ProxyP.Address,"proxy.tencent.com");
		ProxyP.Port=8080;
	}

	//Log("��������Ŀ���ַ�߳�");
	pChildThread = CreateThread(NULL, 0, ProxyToServer, (LPVOID)&ProxyP, NULL, NULL);

	//Log("�ȴ�����Ŀ���ַ�¼�");
	::WaitForSingleObject(ProxyP.IsConnectedOK,60000);
	::CloseHandle(ProxyP.IsConnectedOK);
	bool first=true;

	while(SPair.IsProxyToServClosed == FALSE && SPair.IsProxyToUserClosed == FALSE)
	{	
		//Log("��Ŀ���ַ��������");
		if(ProxyP.Port!=443||first==false){
			retval = send(SPair.ProxyToServSocket,Buffer,Len,0);

			if(retval==SOCKET_ERROR)
			{ 
				if(SPair.IsProxyToServClosed == FALSE)
				{
					SPair.IsProxyToServClosed = TRUE;
					closesocket(SPair.ProxyToServSocket);
				}
				return 0;
			}
		}
		first=false;

		//Log("���û���ַ��������");
	    retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);
	 
		if(retval==SOCKET_ERROR)
		{
			//Log("\nError Recv"); 
			if(SPair.IsProxyToUserClosed==FALSE)
			{
				SPair.IsProxyToUserClosed=TRUE;
				closesocket(SPair.ProxyToUserSocket);
			}
			continue;
		}

		if(retval==0)
		{
			//Log("Client Close connection\n");
		    if(SPair.IsProxyToUserClosed==FALSE)
			{
				closesocket(SPair.ProxyToUserSocket);
				SPair.IsProxyToUserClosed=TRUE;
			}
			break;
		}
		Len=retval;
	}	//End While

	if(SPair.IsProxyToServClosed == FALSE)
	{
		closesocket(SPair.ProxyToServSocket);
		SPair.IsProxyToServClosed=TRUE;
	}
	if(SPair.IsProxyToUserClosed == FALSE)
	{
		closesocket(SPair.ProxyToUserSocket);
		SPair.IsProxyToUserClosed=TRUE;
	}
	::WaitForSingleObject(pChildThread,INFINITE);
	CloseHandle(pChildThread);
	
	return 0;
}

// ��ȡԶ���������ݣ����������ؿͻ���
DWORD WINAPI ProxyToServer(LPVOID pParam)
{
    ProxyParam * pPar=(ProxyParam*) pParam;
	char Buffer[MAXBUFFERSIZE];

	char * server_name;
	unsigned int addr;
	int socket_type ;
	unsigned short port ;
	int retval,Len;
	struct sockaddr_in server;
	struct hostent *hp;
	SOCKET conn_socket;
	//CString strLog;

	server_name = pPar->Address;
	socket_type = SOCK_STREAM;
	port = pPar->Port;
	if (isalpha(server_name[0])) 
	{
		/* server address is a name */
		hp = gethostbyname(server_name);
		//Log("GetHostByName");
	}
	else  
	{
		/* Convert nnn.nnn address to a usable one */
		addr = inet_addr(server_name);
		hp = gethostbyaddr((char *)&addr,4,AF_INET);
		//Log("GetHostByName");
	}

	if (hp == NULL ) 
	{
		//strLog.Format("Client: Cannot resolve address [%s]: Error %d\n",server_name,WSAGetLastError());
		//Log(strLog);

		::SetEvent(pPar->IsConnectedOK);	
		return 0;
	}
	// Copy the resolved information into the sockaddr_in structure
	//

	memset(&server,0,sizeof(server));
	memcpy(&(server.sin_addr),hp->h_addr,hp->h_length);
	server.sin_family = hp->h_addrtype;
	server.sin_port = htons(port);
	conn_socket = socket(AF_INET,socket_type,IPPROTO_TCP);/* ��һ�� socket */

	if (conn_socket < 0 ) 
	{
		fprintf(stderr,"Client: Error Opening socket: Error %d\n",WSAGetLastError());
		//strLog.Format("Client: Error Opening socket: Error %d\n",WSAGetLastError());
		//Log(strLog);

		pPar->pPair->IsProxyToServClosed=TRUE;
		::SetEvent(pPar->IsConnectedOK);	
		return -1;
	}

	//Log("���ӷ�����. . .");
	if (connect(conn_socket,(struct sockaddr*)&server,sizeof(server))== SOCKET_ERROR) 
	{
	//
		//strLog.Format("connect() failed: %d\n",WSAGetLastError());
		pPar->pPair->IsProxyToServClosed=TRUE;	
		::SetEvent(pPar->IsConnectedOK);	
		//return -1;
	}
	//Log("���ӷ������ɹ�");
	if(port==443){
		char* buffer="HTTP/1.1 200 Connection established\r\n\r\n";
		retval = send(pPar->pPair->ProxyToUserSocket,buffer,strlen(buffer),0);
		if (retval == 0) 
		{
			fprintf(stderr,"recv() failed: error %d\n",WSAGetLastError());
		}
	}

	pPar->pPair->ProxyToServSocket = conn_socket;
	pPar->pPair->IsProxyToServClosed = FALSE;
    ::SetEvent(pPar->IsConnectedOK);

	while(!pPar->pPair->IsProxyToServClosed && !pPar->pPair->IsProxyToUserClosed)
	{
		//Log("׼������Ŀ���ַ������");
		retval = recv(conn_socket,Buffer,sizeof (Buffer),0 );
		//Log("����Ŀ���ַ������");

		if (retval == SOCKET_ERROR ) 
		{
			fprintf(stderr,"recv() failed: error %d\n",WSAGetLastError());
			closesocket(conn_socket);
			if(!pPar->pPair->IsProxyToServClosed){
				pPar->pPair->IsProxyToServClosed=TRUE;
			}
			break;
		}

		Len=retval;
		if (retval == 0) 
		{
			//Log("Server closed connection\n");
			pPar->pPair->IsProxyToServClosed=TRUE;
			closesocket(conn_socket);
			break;
		}

		//Log("׼�����û���ַ��������");
		retval = send(pPar->pPair->ProxyToUserSocket,Buffer,Len,0);
		if (retval == SOCKET_ERROR) 
		{
			fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
			pPar->pPair->IsProxyToServClosed=TRUE;
			closesocket(pPar->pPair->ProxyToUserSocket);
			break;						
		}
		//Log("���û���ַ��������");
	}

	if(pPar->pPair->IsProxyToServClosed==FALSE)
	{
		closesocket(pPar->pPair->ProxyToServSocket);
		pPar->pPair->IsProxyToServClosed=TRUE;
	}
	if(pPar->pPair->IsProxyToUserClosed==FALSE)
	{
		closesocket(pPar->pPair->ProxyToUserSocket);
		pPar->pPair->IsProxyToUserClosed=TRUE;
	}
	::SetEvent(pPar->IsExit);	
	return 1;
}
#endif
