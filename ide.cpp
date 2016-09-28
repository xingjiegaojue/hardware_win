#include <Windows.h>
#include <stdio.h>

#pragma comment (lib,"ws2_32.lib") 

#pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

#include "hardware.h"
#include "csharememory.h"

#define PORT					6000
#define SHARE_MEM_SIZE			32

void execCycle()
{
	WORD wVersionRequested;  
	WSADATA wsaData;  
	int err;  
	wVersionRequested = MAKEWORD(1,1);
	err = WSAStartup( wVersionRequested, &wsaData );  
	if (err != 0 ) {  
		return;
	}  

	if (LOBYTE( wsaData.wVersion ) != 1 ||  
		HIBYTE( wsaData.wVersion ) != 1) {  
			WSACleanup( );  
			return;   
	}  

	SOCKET svr = socket(AF_INET,SOCK_DGRAM,0);  
	SOCKADDR_IN addr;  
	addr.sin_family = AF_INET;  
	addr.sin_port = htons(PORT);  
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  
	int len = sizeof(sockaddr);  
	bind(svr,(sockaddr*)&addr,len);  

	SOCKADDR_IN addrClient;  
	while(true)  {  
		char recvBuf[128] = { 0 };  
		recvfrom(svr,recvBuf,128,0,(sockaddr*)&addrClient,&len);  
		if(strcmp(recvBuf,"stop") == 0) {
			break;
		}
	}  
	closesocket(svr);  
	WSACleanup();  
}

int main(int argc,char *argv[])
{
	unsigned char serialNumberBytes[64] = { 0 };
	getSerialNumber(serialNumberBytes);

	for(int i = 0;i < 64;i++) {
		//printf("%02x ",serialNumberBytes[i]);
	}

	TCHAR chKey[16] = { 0 };
	chKey[0] = 0x68;
	chKey[1] = 0x61;
	chKey[2] = 0x72;
	chKey[3] = 0x64;
	chKey[4] = 0x77;
	chKey[5] = 0x61;
	chKey[6] = 0x72;
	chKey[7] = 0x65;
	
	CShareMemory *pShareMem = new CShareMemory;
	if(pShareMem->Open(chKey,SHARE_MEM_SIZE)) {
		pShareMem->Write(serialNumberBytes,SHARE_MEM_SIZE);
	}

	execCycle();

	delete pShareMem;

	return 0;
}

