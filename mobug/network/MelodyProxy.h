#ifndef _PROXY_H
#define _PROXY_H

#include <WinSock2.h>
#include "windows.h"
#include <iostream>
#include <malloc.h>
#include <process.h>
#include <Winhttp.h>
#include "../myutil.h"
#include "../transparent_wnd.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winhttp.lib")
using namespace std;

#define MAXBUFFERSIZE      1024*20   //��������С
#define HTTP  "http://"
#define HTTPS  "CONNECT "

#define DEBUG 1

//�궨��
#define BLOCK 1024
#define MAXNUM 8
#define MAX_LINK_COUNT 100

class MelodyProxy{
public:
	//Proxy �˿�
	CRITICAL_SECTION cs;		//�ٽ��
	UINT pport;// = 8888;
	bool agentRequest;
	bool replaceResponse;
	TransparentWnd* winHandler;
	static int linkCount;
	static map<wstring, wstring> proxyMap;
	MelodyProxy(){
		pport=8888;
		agentRequest=false;
		replaceResponse=false;
	}
	MelodyProxy(TransparentWnd* _winHandler, UINT _pport=8888, bool _replaceResponse=false){
		pport=_pport;
		agentRequest=true;
		replaceResponse=_replaceResponse;
		winHandler=_winHandler;
	}

	struct ProxySockets
	{
		SOCKET  ProxyToUserSocket;	//���ػ�����PROXY �������socket
		SOCKET  ProxyToServSocket;	//PROXY �������Զ��������socket
		BOOL    IsProxyToUserClosed; // ���ػ�����PROXY �����״̬
		BOOL    IsProxyToServClosed; // PROXY �������Զ������״̬
	};

	static struct ProxyParam
	{
		char Address[256];		// Զ��������ַ
		char Cmd[10];		// Զ��������ַ
		HANDLE IsConnectedOK;	// PROXY �������Զ������������״̬
		HANDLE IsExit;	// PROXY �������Զ������������״̬
		HANDLE ReplaceResponseOK;	// PROXY �������Զ������������״̬
		HANDLE ReplaceResponseEnd;	// PROXY �������Զ������������״̬
		HANDLE ResponseOK;	// PROXY �������Զ������������״̬
		HANDLE ResponseEnd;	// PROXY �������Զ������������״̬
		bool CancelReplaceResponse;
		bool cancelResponse;
		bool isReplaceRequest;
		bool connectionClose;
		ProxySockets * pPair;	// ά��һ��SOCKET��ָ��
		void * pProxy;	// ά��һ��SOCKET��ָ��
		int Port;			// ��������Զ�������Ķ˿�
		char Request[MAXBUFFERSIZE];
		char* Response;
		char* replaceResponse;
		bool isHttps;
		int retryCount;
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
		//char ip[100];
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons(pport);
		//
		listentsocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
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
		if(::listen(listentsocket,5000)!=0)
		{
			//Log("\n Error in Listen.");
			WSACleanup(); 
			return -4;
		}
		//	::listentsocket=listentsocket; 
		//InitializeMemPool();
		//Log("���������߳�");
		HANDLE threadhandle = (HANDLE)_beginthreadex(NULL, 0, UserToProxy, (LPVOID)this, NULL, NULL);
		CloseHandle(threadhandle);
		InitializeCriticalSection(&cs);
		return 1;
	}

	static void GetIP(char *ip, const char* hostName){
		BYTE   *p;   
		struct   hostent   *hp; 
		//char   ip[16]; 

		if((hp   =gethostbyname(hostName))!=0) 
		{ 
			p   =(BYTE   *)hp-> h_addr;   
			sprintf(ip,   "%d.%d.%d.%d ",   p[0],   p[1],   p[2],   p[3]); 
			cout<<ip<<"\n";
		} 
	}

	static const char* GetLocalIP(char* ip){
		char   temp[100];
		if(gethostname(temp, sizeof(temp))==0){
			GetIP(ip, temp);
		}
		return ip;
	}


	int CloseServer() //�رշ���
	{
		closesocket(listentsocket);
		//	WSACleanup();
		return 1;
	}

	~MelodyProxy(){
		closesocket(listentsocket);
		DeleteCriticalSection(&cs);
	}
	//�������յ����ַ����õ�Զ��������ַ
	static int ResolveInformation( char * str, char* command, char *address, int * port)
	{
		char buf[MAXBUFFERSIZE], proto[128], *p=NULL;

		//CString strLog;

		sscanf(str,"%s%s%s",command,buf,proto);
		//strcpy(url, buf);
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
			/*for(UINT j=0;j< i+strlen(HTTP);j++)
			{
			*(p+j)=' ';
			}*/

			return 1;
		}
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

			return 1;
		}

		return 0; 
	}
	static void logSocketData(MelodyProxy* proxy, char* tag, char* content, int length=0){
		char buffer[1000];
		sprintf(buffer,"log/%ld%s%s.txt",proxy,tag,GetTime().c_str());
		string path=buffer;
		proxy->winHandler->WriteFile(path, content, length);
	}
	static int readContentLength(char* buffer){
		char* p;
		if(p=strstr(buffer,"Content-Length: ")){
			char temp1[100];
			p+=strlen("Content-Length: ");
			int l=strlen(p);
			if(l>30)l=30;
			strncpy(temp1,p,l);
			for(char* i=temp1;i<temp1+30;++i){
				if(i[0]=='\r'){
					i[0]=0;
					break;
				}
			}
			int length=atoi(temp1);
			return length;
		}
		else{
			return -1;
		}
	}
	static wstring GetIEProxy(const std::wstring& strHostName)
	{
		if(proxyMap.find(strHostName) != proxyMap.end()){
			return proxyMap[strHostName];
		}
		std::wstring strRet_cswuyg;
		WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions = {0};
		WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig = {0};
		BOOL bAutoDetect = FALSE; //���Զ�������á�������ʱ�򼴱�ѡ����Ҳ�᷵��0��������Ҫ����url�ж�
		if(::WinHttpGetIEProxyConfigForCurrentUser(&ieProxyConfig))
		{
			if(ieProxyConfig.fAutoDetect)
			{
				bAutoDetect = TRUE;
			}
			if( ieProxyConfig.lpszAutoConfigUrl != NULL )
			{
				bAutoDetect = TRUE;
				autoProxyOptions.lpszAutoConfigUrl = ieProxyConfig.lpszAutoConfigUrl;
			}
		}
		else
		{
			// error
			return strRet_cswuyg;
		}

		if(bAutoDetect)
		{
			if (autoProxyOptions.lpszAutoConfigUrl != NULL)
			{ 
				autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			}
			else
			{
				autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
				autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
			}
			autoProxyOptions.fAutoLogonIfChallenged = TRUE;
			HINTERNET hSession = ::WinHttpOpen(0, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
				WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
			if (hSession != NULL)
			{
				WINHTTP_PROXY_INFO autoProxyInfo = {0};
				bAutoDetect = ::WinHttpGetProxyForUrl(hSession, strHostName.c_str(), &autoProxyOptions, &autoProxyInfo);
				if(autoProxyInfo.lpszProxy!=NULL){
					strRet_cswuyg=autoProxyInfo.lpszProxy;
					proxyMap[strHostName]=strRet_cswuyg;
				}
				else{
					proxyMap[strHostName]=L"";
				}
				if (hSession!= NULL) 
				{
					::WinHttpCloseHandle(hSession);
				}
				if(autoProxyInfo.lpszProxy)
				{
					if (autoProxyInfo.lpszProxyBypass == NULL)
					{
						std::wstring strProxyAddr = autoProxyInfo.lpszProxy;
						//strRet_cswuyg = GetProxyFromString(eProxyType, strProxyAddr);
					}
					if(autoProxyInfo.lpszProxy != NULL)
					{
						GlobalFree(autoProxyInfo.lpszProxy);
					}
					if(autoProxyInfo.lpszProxyBypass !=NULL) 
					{
						GlobalFree(autoProxyInfo.lpszProxyBypass); 
					}
				}
			}
		}
		else
		{
			if(ieProxyConfig.lpszProxy != NULL)
			{
				if(ieProxyConfig.lpszProxyBypass == NULL)
				{
					std::wstring strProxyAddr = ieProxyConfig.lpszProxy;
					proxyMap[strHostName]=strRet_cswuyg;
					//strRet_cswuyg = GetProxyFromString(eProxyType, strProxyAddr);
				}
			}
		}

		if(ieProxyConfig.lpszAutoConfigUrl != NULL)
		{
			::GlobalFree(ieProxyConfig.lpszAutoConfigUrl);
		}
		if(ieProxyConfig.lpszProxy != NULL)
		{
			::GlobalFree(ieProxyConfig.lpszProxy);
		}
		if(ieProxyConfig.lpszProxyBypass != NULL)
		{
			::GlobalFree(ieProxyConfig.lpszProxyBypass);
		}

		return strRet_cswuyg;
	}
	// ȡ�����ص����ݣ�����Զ������
	static unsigned int WINAPI UserToProxy(void *pParam)
	{
		sockaddr_in from;
		int  fromlen = sizeof(from);
		MelodyProxy* proxy=(MelodyProxy *)pParam;

		char Buffer[MAXBUFFERSIZE];
		int  Len;

		SOCKET ProxyToUserSocket;
		ProxySockets SPair;
		ProxyParam   ProxyP;

		HANDLE pChildThread;
		int retval;

		//Log("���������û�����");
		ProxyToUserSocket = accept(proxy->listentsocket,(struct sockaddr*)&from,&fromlen);
		int time_out = 180000; // 3����

		setsockopt(ProxyToUserSocket, SOL_SOCKET,SO_RCVTIMEO, (char *)&time_out,sizeof(time_out));
		setsockopt(ProxyToUserSocket, SOL_SOCKET,SO_SNDTIMEO, (char *)&time_out,sizeof(time_out));
		int rcvbuf;  
		int rcvbufsize=sizeof(int);

		if(getsockopt(ProxyToUserSocket,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&rcvbuf,&rcvbufsize)!=SOCKET_ERROR)
		{
			if(rcvbuf<MAXBUFFERSIZE)
				rcvbuf=MAXBUFFERSIZE;
			setsockopt(ProxyToUserSocket,SOL_SOCKET,SO_RCVBUF,(char*)&rcvbuf,rcvbufsize);
		}

		if(getsockopt(ProxyToUserSocket,SOL_SOCKET,SO_SNDBUF,(char*)&rcvbuf,&rcvbufsize)!=SOCKET_ERROR)
		{
			if(rcvbuf<MAXBUFFERSIZE)
				rcvbuf=MAXBUFFERSIZE;
			setsockopt(ProxyToUserSocket,SOL_SOCKET,SO_SNDBUF,(char*)&rcvbuf,rcvbufsize);
		}		//Log("��������");

		//Log("�����µ������߳�");
		HANDLE threadhandle = (HANDLE)_beginthreadex(NULL, 0, UserToProxy, (LPVOID)pParam, NULL, NULL);
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

		char* bufferAll=new char[MAXBUFFERSIZE*50];
		char* content=new char[MAXBUFFERSIZE*50];
		char* header=new char[MAXBUFFERSIZE];
		int headerLength=0;
		int length=0;
		int count;
		bool replaceResponse=false;
		//Log("�����û�����");
		retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);
		if(retval==SOCKET_ERROR)
		{ 
			//Log("\nError Recv"); 
			if(SPair.IsProxyToUserClosed == FALSE)
			{
				closesocket(SPair.ProxyToUserSocket);
				SPair.IsProxyToUserClosed = TRUE;

			}
			char buffer[1000];
			sprintf(buffer,"receive request failed: error %d\n",WSAGetLastError());
			logSocketData(proxy, "recvrequesterr", buffer);
			return 0;
		}

		if(retval==0)
		{
			//Log("Client Close connection\n");
			if(SPair.IsProxyToUserClosed==FALSE)
			{	
				closesocket(SPair.ProxyToUserSocket);
				SPair.IsProxyToUserClosed=TRUE;

			}
			char buffer[1000];
			sprintf(buffer,"client close: error %d\n",WSAGetLastError());
			logSocketData(proxy, "client close", buffer);
			return 0;
		}
		Len = retval;
		SPair.IsProxyToUserClosed = FALSE;
		SPair.IsProxyToServClosed = TRUE;
		SPair.ProxyToUserSocket = ProxyToUserSocket;
		ProxyP.pPair=&SPair;
		ProxyP.IsConnectedOK = CreateEvent(NULL,TRUE,FALSE,NULL);
		ProxyP.ReplaceResponseOK = CreateEvent(NULL,TRUE,FALSE,NULL);
		ProxyP.ReplaceResponseEnd = CreateEvent(NULL,TRUE,FALSE,NULL);
		ProxyP.ResponseEnd = CreateEvent(NULL,TRUE,FALSE,NULL);
		ProxyP.ResponseOK = CreateEvent(NULL,TRUE,FALSE,NULL);
		ProxyP.pProxy=(void *)proxy;
		ProxyP.retryCount=0;
		memcpy(ProxyP.Request,Buffer, Len);
		ProxyP.Request[Len]=0;

		if(proxy->agentRequest){
			//���Ϊpost���󣬶�ȡ�����������ٴ���
			bool isAgent=false;
			if(!strcmp(ProxyP.Cmd,"POST")){
				length=readContentLength(Buffer);
				char* start=strstr(Buffer,"\r\n\r\n");
				headerLength=start-Buffer+4;
				count=Len-headerLength;
				memcpy(content,start+4,count);
				memcpy(header,Buffer,headerLength);
				//char* end=strstr(header,"\r\n\r\n");
				//end[4]=0;
				while(count<length){
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
						char buffer[1000];
						sprintf(buffer,"receive request failed: error %d\n",WSAGetLastError());
						logSocketData(proxy, "recvrequesterr", buffer);
						break;
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
					Buffer[retval]=0;
					memcpy(content+count,Buffer,retval);
					count+=retval;
					logSocketData(proxy, "request", Buffer);
				}
				content[length]=0;
				if(strlen(content)==(UINT)length){
					content[length]=0;
					//ֻ����Ƕ���������
					proxy->winHandler->agentRequest(header,content,&ProxyP);
					isAgent=true;
				}
				else{
					//�����Ƶ�ֱ�ӷ��ͣ�
				}
				logSocketData(proxy, "requestheader", header);
				logSocketData(proxy, "requestcontent", content, length);
			}
			else{
				isAgent=true;
				proxy->winHandler->agentRequest(Buffer,"",&ProxyP);
				logSocketData(proxy, "request", Buffer);
			}
			if(isAgent){
				::WaitForSingleObject(ProxyP.ResponseOK,INFINITE);
				if(ProxyP.isReplaceRequest){
					strcpy(Buffer, ProxyP.Request);
					Len=strlen(Buffer);
					Buffer[Len]=0;
				}
				else if(!ProxyP.cancelResponse){
					retval = send(ProxyP.pPair->ProxyToUserSocket,ProxyP.Response,strlen(ProxyP.Response),0);
					if(retval==SOCKET_ERROR)
					{ 
						if(SPair.IsProxyToUserClosed == FALSE)
						{
							SPair.IsProxyToUserClosed = TRUE;
							closesocket(SPair.ProxyToUserSocket);
						}
						::PulseEvent(ProxyP.ResponseEnd);
						return 0;
					}
					replaceResponse=true;
				}
				::PulseEvent(ProxyP.ResponseEnd);
			}
		}
		if(ProxyP.Port==443){
			ProxyP.isHttps=true;
		}
		else{
			ProxyP.isHttps=false;
		}

		if(MAXBUFFERSIZE>Len){
			Buffer[Len]=0;
		}
		ProxyP.pProxy=(void *)proxy;
		struct hostent *hp;
		ResolveInformation( Buffer,ProxyP.Cmd, ProxyP.Address,&ProxyP.Port);
		hp = gethostbyname("proxy.tencent.com");


		//Log("�����û�����");
		if(hp!=NULL){
			char url[1000]={0};
			if(ProxyP.Port==443){
				strcpy(url,"https://");
			}
			else{
				strcpy(url,"http://");
			}
			strcat(url,ProxyP.Address);
			CefString s=url;
			CefString proxyStr=GetIEProxy(s.ToWString());
			string ieProxy=proxyStr.ToString();
			int index=ieProxy.find_last_of(":");
			if(index!=-1){
				ProxyP.Port=atoi(ieProxy.substr(index+1).c_str());
				strcpy(ProxyP.Address,ieProxy.substr(0,index).c_str());
			}
			else{
				strcpy(ProxyP.Address,"proxy.tencent.com");
				ProxyP.Port=8080;
			}
		}

		//Log("��������Ŀ���ַ�߳�");
		pChildThread = (HANDLE)_beginthreadex(NULL, 0, ProxyToServer, (LPVOID)&ProxyP, 0, NULL);

		//Log("�ȴ�����Ŀ���ַ�¼�");
		::WaitForSingleObject(ProxyP.IsConnectedOK,60000);
		::CloseHandle(ProxyP.IsConnectedOK);
		bool first=true;

		while(SPair.IsProxyToServClosed == FALSE && SPair.IsProxyToUserClosed == FALSE)
		{	
			//Log("��Ŀ���ַ��������");
			//https���״ν�͸�������߷�https����û���滻
			if((ProxyP.isHttps&&!first)||(!ProxyP.isHttps&&!replaceResponse)){
				//�滻proxy-connection;
				Buffer[Len]=0;
				if(length){
					if(strstr(header,"\r\nProxy-Connection: ")){
						char Buffer2[MAXBUFFERSIZE];
						replace(header,"\r\nProxy-Connection: ","\r\nConnection: ", Buffer2, MAXBUFFERSIZE);
						strcpy(header,Buffer2);
						headerLength=strlen(Buffer);
					}
				}
				else if(strstr(Buffer,"\r\nProxy-Connection: ")){
					char Buffer2[MAXBUFFERSIZE];
					replace(Buffer,"\r\nProxy-Connection: ","\r\nConnection: ", Buffer2, MAXBUFFERSIZE);
					strcpy(Buffer,Buffer2);
					Len=strlen(Buffer);
				}
				//Len=retval;
				//post����
				if(length){
					memcpy(bufferAll, header, headerLength);
					int totalLength=headerLength+length;
					memcpy(bufferAll+headerLength, content, length);
					bufferAll[totalLength]=0;
					retval = send(SPair.ProxyToServSocket,bufferAll,totalLength,0);
					logSocketData(proxy, "sendrequestall", bufferAll,totalLength);
				}
				else{
					retval = send(SPair.ProxyToServSocket,Buffer,Len,0);
					logSocketData(proxy, "sendrequest", Buffer, Len);
				}
				if(retval==SOCKET_ERROR)
				{
					//continue;
					if(SPair.IsProxyToServClosed == FALSE)
					{
						//SPair.IsProxyToServClosed = TRUE;
						//closesocket(SPair.ProxyToServSocket);
						//��������֮���Ƿ���Ҫ�����˳���
					}
					char buffer[1000];
					sprintf(buffer,"send request failed: error %d\n",WSAGetLastError());
					logSocketData(proxy, "sendrequesterr", buffer);
					break;
				}
			}
			first=false;

			//Log("���û���ַ��������");��ȡhttp header����
			retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);

			if(retval==SOCKET_ERROR)
			{
				//Log("\nError Recv"); 
				//retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);
				if(SPair.IsProxyToUserClosed==FALSE)
				{
					//�Ƿ�Ӧ�����Ϲر�socket;
					//SPair.IsProxyToUserClosed=TRUE;
					//closesocket(SPair.ProxyToUserSocket);
				}
				Len=0;
				char buffer[1000];
				sprintf(buffer,"receive request failed: error %d\n",WSAGetLastError());
				logSocketData(proxy, "recvrequesterr", buffer);
				break;
			}

			if(retval==0)
			{
				//Log("Client Close connection\n");
				if(SPair.IsProxyToUserClosed==FALSE)
				{
					//closesocket(SPair.ProxyToUserSocket);
					//SPair.IsProxyToUserClosed=TRUE;
				}
				//MessageBoxA(NULL,"client close", "", 0);
				logSocketData(proxy,"clientclose","client close");
				break;
			}
			Len=retval;
			Buffer[Len]=0;
			replaceResponse=false;
			if(proxy->agentRequest){
				//���Ϊpost���󣬶�ȡ�����������ٴ���
				length=0;
				bool isAgent=false;
				if(strstr(Buffer,"POST")==Buffer){
					length=readContentLength(Buffer);
					char* start=strstr(Buffer,"\r\n\r\n");
					headerLength=start-Buffer+4;
					count=Len-headerLength;
					memcpy(content,start+4,count);
					memcpy(header,Buffer,headerLength);
					header[headerLength]=0;
					while(count<length){
						//Log("���û���ַ��������");
						retval = recv(SPair.ProxyToUserSocket,Buffer,sizeof(Buffer),0);

						if(retval==SOCKET_ERROR)
						{
							//Log("\nError Recv"); 
							if(SPair.IsProxyToUserClosed==FALSE)
							{
								//�Ƿ���Ҫ�ر�socket
								//SPair.IsProxyToUserClosed=TRUE;
								//closesocket(SPair.ProxyToUserSocket);
							}
							char buffer[1000];
							sprintf(buffer,"receive request failed: error %d\n",WSAGetLastError());
							logSocketData(proxy, "recvrequesterr", buffer);
							break;
						}

						if(retval==0)
						{
							//Log("Client Close connection\n");
							if(SPair.IsProxyToUserClosed==FALSE)
							{
								closesocket(SPair.ProxyToUserSocket);
								SPair.IsProxyToUserClosed=TRUE;
							}
							logSocketData(proxy,"clientclose","client close");
							break;
						}
						logSocketData(proxy,"request",Buffer,retval);
						memcpy(content+count,Buffer,retval);
						count+=retval;
					}
					if(strlen(content)==(UINT)length){
						content[length]=0;
						proxy->winHandler->agentRequest(header,content,&ProxyP);
						isAgent=true;
					}
					logSocketData(proxy,"requestheader",header,headerLength);
					logSocketData(proxy,"requestcontent",content,length);
				}
				else{
					proxy->winHandler->agentRequest(Buffer,"",&ProxyP);
					logSocketData(proxy,"request",Buffer);
					isAgent=true;
				}
				if(isAgent){
					::WaitForSingleObject(ProxyP.ResponseOK,INFINITE);
					if(ProxyP.isReplaceRequest){
						strcpy(Buffer, ProxyP.Request);
						Len=strlen(Buffer);
					}
					else if(!ProxyP.cancelResponse){
						retval = send(ProxyP.pPair->ProxyToUserSocket,ProxyP.Response,strlen(ProxyP.Response),0);
						if(retval==SOCKET_ERROR)
						{ 
							if(SPair.IsProxyToUserClosed == FALSE)
							{
								//SPair.IsProxyToUserClosed = TRUE;
								//closesocket(SPair.ProxyToUserSocket);
								//MessageBoxA(NULL,"close5","close5",0);
							}
							logSocketData(proxy,"replaceresponseerr",ProxyP.Response);
							::PulseEvent(ProxyP.ResponseEnd);
							break;
						}
						logSocketData(proxy,"replaceresponse",ProxyP.Response);
						replaceResponse=true;
					}
					::PulseEvent(ProxyP.ResponseEnd);
				}
			}
		}
		//End While

		::WaitForSingleObject(pChildThread,INFINITE);
		CloseHandle(pChildThread);
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

		return 0;
	}

	static int readChunk(char* str, int len, char** pBuffer, MelodyProxy* pProxy){
		if(len==0){
			return 0;
		}
		char* buffer=*pBuffer;
		char temp1[10]={"0x"};
		int res=len;//����ʣ���ֽ���
		char* seek=strstr(str,"\r\n");//����16�����ַ��������CRLF
		if(!seek){
			//MessageBoxA(NULL,"eror","error",0);
			return 0;
		}
		int l1=seek-str;//16�����ַ�������
		strncpy(temp1+2,str,l1);//����16�����ַ���
		temp1[l1+2]=0;//��0
		char* strEnd;
		//pProxy->winHandler->agentResponse(str);
		int i = (int)strtol(temp1, &strEnd, 16);//ʮ������
		res-=l1+2;
		if(i==0){
			return -1;
		}
		if(res<=0){
			return i;
		}
		//����һ��chunk��
		if(res<i){
			//�������ݵ�content��
			memcpy(buffer,seek+2,res);//���Ƶ���������
			buffer[res]=0;
			*pBuffer=buffer+res;
			return i-res;//����ʣ���chunk���ݵĳ���
		}
		else{
			memcpy(buffer,seek+2,i);//���Ƶ���������
			//��һ��chunk��ָ�룬ʣ���ַ������ȣ����¼��㻺������ʼλ��
			buffer[i]=0;
			*pBuffer=buffer+i;
			res-=i+2;
			if(res>0){
				return readChunk(seek+2+i+2,res,pBuffer,pProxy);
			}
			else{
				return 0;
			}
		}
	}
	static char *replace(char *st, char *orig, char *repl, char* buffer, int l) {
		ZeroMemory(buffer,l);
		char *ch;
		if (!(ch = strstr(st, orig))){
			strcpy(buffer, st);
			return buffer;
		}
		strncpy(buffer, st, ch-st);  
		buffer[ch-st] = 0;
		sprintf(buffer+(ch-st), "%s%s", repl, ch+strlen(orig));
		return buffer;
	}
	static unsigned int WINAPI ProxyToServer(LPVOID pParam);
	static int Response(char* contentType, char* header, char* content, int count, MelodyProxy* pProxy, ProxyParam* pPar);
};
#endif
