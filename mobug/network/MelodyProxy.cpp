#include "MelodyProxy.h"
#include "gzip.h"
int MelodyProxy::linkCount = 0;
map<wstring, wstring> MelodyProxy::proxyMap;

// 读取远程主机数据，并发往本地客户机
unsigned int MelodyProxy::ProxyToServer(LPVOID pParam)
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
	conn_socket = socket(AF_INET,socket_type,IPPROTO_TCP);/* 打开一个 socket */
	MelodyProxy *proxy=(MelodyProxy *)pPar->pProxy;

	if (conn_socket < 0 ) 
	{
		//strLog.Format("Client: Error Opening socket: Error %d\n",WSAGetLastError());
		//Log(strLog);

		pPar->pPair->IsProxyToServClosed=TRUE;
		::SetEvent(pPar->IsConnectedOK);	
		char buffer[1000];
		sprintf(buffer,"%sClient: Error Opening socket: Error %d\n",server_name,WSAGetLastError());
		logSocketData(proxy, "opensocketerror", buffer);
		return -1;
	}

	//Log("连接服务器. . .");
	if (connect(conn_socket,(struct sockaddr*)&server,sizeof(server))== SOCKET_ERROR) 
	{
	//
		//strLog.Format("connect() failed: %d\n",WSAGetLastError());
		pPar->pPair->IsProxyToServClosed=TRUE;	
		::SetEvent(pPar->IsConnectedOK);	
		char buffer[1000];
		sprintf(buffer,"%sconnect error %d\n",server_name,WSAGetLastError());
		logSocketData(proxy,"connecterror",buffer);
		//return -1;
		Sleep(100);
	}
	int rcvbuf;  
	int rcvbufsize=sizeof(int);

	if(getsockopt(conn_socket,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&rcvbuf,&rcvbufsize)!=SOCKET_ERROR)
	{
		if(rcvbuf<MAXBUFFERSIZE)
				rcvbuf=MAXBUFFERSIZE;
		setsockopt(conn_socket,SOL_SOCKET,SO_RCVBUF,(char*)&rcvbuf,rcvbufsize);
	}

	if(getsockopt(conn_socket,SOL_SOCKET,SO_SNDBUF,(char*)&rcvbuf,&rcvbufsize)!=SOCKET_ERROR)
	{
		if(rcvbuf<MAXBUFFERSIZE)
			rcvbuf=MAXBUFFERSIZE;
		setsockopt(conn_socket,SOL_SOCKET,SO_SNDBUF,(char*)&rcvbuf,rcvbufsize);
	}		//Log("接受连接");

	//Log("连接服务器成功");
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
	int length=-1;
	char header[MAXBUFFERSIZE]={0};
	char* content = new char[MAXBUFFERSIZE*50];
	ZeroMemory(content,MAXBUFFERSIZE*50);
	char* endContentP=content;
	bool isChunked=false;
	bool isContentLength=false;
	int resChunkCount=0;
	bool isFirst=true;
	pPar->Response=NULL;
	bool isSend=true;
	int maxtry=0;
	int count=0;
	int headerLength;
	char contentType[100]={0};
	while(!pPar->pPair->IsProxyToServClosed && !pPar->pPair->IsProxyToUserClosed)
	{
		//Log("准备接收目标地址的数据");
		retval = recv(conn_socket,Buffer,sizeof (Buffer),0 );
		//Log("接收目标地址的数据");

		if (retval == SOCKET_ERROR ) 
		{
			char buffer[1000];
			sprintf(buffer,"recv response failed: error %d\n",WSAGetLastError());
			//pProxy->winHandler->agentRequest(pPar->Request);

			//Log("连接服务器. . .");
			/*if(!pPar->pPair->IsProxyToServClosed){
				pPar->pPair->IsProxyToServClosed=TRUE;
				closesocket(conn_socket);
			}*/
			logSocketData(proxy,"recvresponseerr","server close");
			break;
		}

		if (retval == 0) 
		{
			//Log("Server closed connection\n");
			closesocket(conn_socket);
			pPar->pPair->IsProxyToServClosed=TRUE;
			logSocketData(proxy,"serverclose","server close");
			break;
		}
		Len=retval;
		if(Len<MAXBUFFERSIZE){
			Buffer[Len]=0;
		}
		logSocketData(proxy,"response",Buffer,Len);
		//第一次收到包分析头部
		if(isFirst&&!pPar->isHttps){
			//先判断是否有Transfer-Encoding: chunked
			char* start=strstr(Buffer,"\r\n\r\n");
			while(start){
				char* p = strstr(Buffer,"Content-Type: ");
				if(p){
					p+=strlen("Content-Type: ");
					char* end=strstr(p,"\r\n");
					if(end){
						strncpy(contentType,p,end-p);
					}
				}
				/*if(!strstr(contentType,"text")&&!strstr(contentType,"javascript")){
					break;
				}*/
				//分割头部
				headerLength=start-Buffer+4;//头部长度
				strncpy(header,Buffer,headerLength);
				header[headerLength]=0;
				p = strstr(Buffer,"Transfer-Encoding: chunked");
				//chunked传输
				if(p){
					if(Len-headerLength>0){
						resChunkCount=readChunk(start+4,Len-headerLength,&endContentP,proxy);
					}
					isChunked=true;
				}
				//判断是否有Content-Length
				else if(p=strstr(Buffer,"Content-Length: ")){
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
					length=atoi(temp1);
					isContentLength=true;
					count=Len-headerLength;
					if(count>0){
						memcpy((void *)content,start+4,count);
						content[count]=0;
					}
				}
				isFirst=false;
				break;
			}
			if(!start){
				//MessageBoxA(NULL,Buffer,Buffer,0);
			}
		}
		else if(isChunked){
			if(resChunkCount>0){
				if(Len>resChunkCount){
					memcpy((void *)endContentP,Buffer,resChunkCount);
					endContentP+=resChunkCount;
					resChunkCount=readChunk(Buffer+resChunkCount+2,Len-resChunkCount-2,&endContentP,proxy);
				}
				else{
					memcpy((void *)endContentP,Buffer,Len);
					endContentP+=Len;
					resChunkCount-=Len;
				}
			}
			else{
				resChunkCount=readChunk(Buffer,Len,&endContentP,proxy);
			}
		}
		else if(isContentLength){
			memcpy(content+count,Buffer,Len);
			count+=Len;
		}
		if(isChunked){
			if(resChunkCount==-1){
				isChunked=false;
				int l=endContentP-content;
				char buf[100];
				sprintf(buf,"%s%d","Content-Length: ",l);
				char header2[MAXBUFFERSIZE];
				replace(header,"Transfer-Encoding: chunked",buf, header2,MAXBUFFERSIZE);
				retval=Response(contentType, header2, content, l, proxy, pPar);
				if(retval == SOCKET_ERROR){
					break;
				}
				isFirst=true;
				isChunked=false;
				resChunkCount=0;
				isContentLength=false;
				length=-1;
				endContentP=content;
			}
		}
		else if(isContentLength){
			if(count>=length){
				retval=Response(contentType, header, content, count, proxy, pPar);
				if(retval == SOCKET_ERROR){
					break;
				}
				int res=count-length;
				if(res>0){
					logSocketData(proxy,"datalengtherrheader",header,headerLength);
					logSocketData(proxy,"datalengtherrcontent",content,length);
				}
				count=0;
				isFirst=true;
				isContentLength=false;
				length=-1;
				isChunked=false;
				resChunkCount=0;
				endContentP=content;
			}
		}
		else{
			retval = send(pPar->pPair->ProxyToUserSocket,Buffer,Len,0);
			if (retval == SOCKET_ERROR) 
			{
				char buffer[1000];
				sprintf(buffer,"send response failed: error %d\n",WSAGetLastError());
				logSocketData(proxy,"sendresponseerr",buffer);
				//fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
				//closesocket(pPar->pPair->ProxyToUserSocket);
				//pPar->pPair->IsProxyToUserClosed=TRUE;
				break;						
			}
		}
	}

	if(pPar->pPair->IsProxyToServClosed==FALSE)
	{
		closesocket(pPar->pPair->ProxyToServSocket);
		pPar->pPair->IsProxyToServClosed=TRUE;
	}
	if(pPar->pPair->IsProxyToUserClosed==FALSE)
	{
		//是否需要关闭
		//closesocket(pPar->pPair->ProxyToUserSocket);
		//pPar->pPair->IsProxyToUserClosed=TRUE;
	}
	return 1;
}
int MelodyProxy::Response(char* contentType, char* header, char* content, int count, MelodyProxy* pProxy, ProxyParam* pPar){
	char* bufferAll=new char[MAXBUFFERSIZE*50];
	int retval=0;
	//如果是gzip，先解码
	char *p=strstr(header,"Content-Encoding: gzip");
	uLong contentLength=count;
	char header3[MAXBUFFERSIZE]={0},header4[MAXBUFFERSIZE]={0};
	if(p){
		contentLength=MAXBUFFERSIZE*50;
		Byte* data=new Byte[MAXBUFFERSIZE*50];
		httpgzdecompress((Byte*)content, count, data, &contentLength);
		char buf[100],buf1[100];
		sprintf(buf,"%s%ld", "Content-Length: ",contentLength);
		sprintf(buf1,"%s%d", "Content-Length: ",count);
		replace(header,buf1,buf, header3, MAXBUFFERSIZE);
		replace(header3,"Content-Encoding: gzip\r\n","",header4,MAXBUFFERSIZE);
		header=header4;
		memcpy(content,data,contentLength);
		content[contentLength]=0;
		delete data;
	}
	strcpy(bufferAll,header);
	int headerLength=strlen(header);
	memcpy(bufferAll+headerLength,content,contentLength);
	//MelodyProxy *proxy=(MelodyProxy *)pPar->pProxy;
	bool isText=strstr(contentType,"text")||strstr(contentType,"javascript");
	bool replaceResponse=false;
	if(pProxy->replaceResponse&&isText){
		pProxy->winHandler->agentResponse(pPar->Request,header,content,pPar);
		::WaitForSingleObject(pPar->ReplaceResponseOK,INFINITE);
		if(pPar->CancelReplaceResponse){
			if(pPar->pPair->IsProxyToUserClosed){
				logSocketData(pProxy,"responsererr",bufferAll);
			}
			retval = send(pPar->pPair->ProxyToUserSocket,bufferAll,headerLength+contentLength,0);
			logSocketData(pProxy,"cancelreplaceresponse",bufferAll);
		}
		else{
			//retval = send(pPar->pPair->ProxyToUserSocket,bufferAll,strlen(header)+contentLength,0);
			if(pPar->pPair->IsProxyToUserClosed){
				logSocketData(pProxy,"responsererr",bufferAll);
			}
			retval = send(pPar->pPair->ProxyToUserSocket,pPar->replaceResponse,strlen(pPar->replaceResponse),0);
			replaceResponse=true;
			logSocketData(pProxy,"replaceresponse",pPar->replaceResponse);
			//MessageBoxA(NULL,"response", "response", 0);
		}
		::PulseEvent(pPar->ReplaceResponseEnd);
	}
	else{
		retval = send(pPar->pPair->ProxyToUserSocket,bufferAll,strlen(header)+contentLength,0);
	}
	if (retval == SOCKET_ERROR) 
	{
		fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
		//closesocket(pPar->pPair->ProxyToUserSocket);
		//pPar->pPair->IsProxyToUserClosed=TRUE;
		if(replaceResponse){
			char buffer[1000];
			sprintf(buffer,"send response all failed: error %d\n",WSAGetLastError());
			logSocketData(pProxy,"replaceresponseerr",buffer);
		}
		else{
			char buffer[1000];
			sprintf(buffer,"send response all failed: error %d\n",WSAGetLastError());
			logSocketData(pProxy,"sendresponseallerr",buffer);
		}
		//MessageBoxA(NULL,"err","",0);
	}
	if(pPar->connectionClose){
		//closesocket(pPar->pPair->ProxyToUserSocket);
		//pPar->pPair->IsProxyToUserClosed=TRUE;
	}
	delete []bufferAll;
	return retval;
}