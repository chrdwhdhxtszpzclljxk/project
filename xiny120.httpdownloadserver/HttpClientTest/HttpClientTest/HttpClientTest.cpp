// HttpClientTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <WinSock2.h>
#include <thread>
#pragma comment( lib, "ws2_32.lib" )
#pragma warning(disable:4996)


void threadfiletrans(int sleep){
	WSADATA wsaData; int Result;
	if ((Result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){
		printf("WSAStartup failed with error %d\n", Result);
		return;
	}
	//char* url[] = {"GET /get.asp?key={32DCE648-9DF3-4EED-B0FB-0C811E6D108B} HTTP/1.1\r\n\r\n",
	//	"GET /get.asp?key={BA0810A3-B71C-4302-A903-91E4CD37B44F} HTTP/1.1\r\n\r\n",
	//	"GET /get.asp?key={79974700-F33F-4F84-8CFD-C797394A4814} HTTP/1.1\r\n\r\n"};

	char* url[] = {"GET /get.asp?key={900504FD-B2A8-4ECB-89E4-E3AD6FC15453} HTTP/1.1\r\n\r\n",
		"GET /get.asp?key={900504FD-B2A8-4ECB-89E4-E3AD6FC15453} HTTP/1.1\r\n\r\n",
		"GET /get.asp?key={900504FD-B2A8-4ECB-89E4-E3AD6FC15453} HTTP/1.1\r\n\r\n"};


	while (true){

		SOCKET s;
		struct sockaddr_in addr = { 0 };
		char buffer[10240];
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			perror("socket");
			//exit(1);
		}
		/* 填写sockaddr_in结构*/
		addr.sin_family = AF_INET;
		addr.sin_port = htons(80);
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		/* 尝试连线*/
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0){
			printf("%d\r\n", GetLastError());
			Sleep(100);
			//perror("connect");
			//exit(1);
		}
		/* 接收由server端传来的信息*/
		srand(GetTickCount());
		int id = rand() % 3;
		char* req = url[id];
		send(s, req, strlen(req), 0);
		memset(buffer, 0, sizeof(buffer));
		recv(s, buffer, sizeof(buffer), 0);
		printf(buffer);
		while (1){
			if (recv(s, buffer, sizeof(buffer), 0) <= 0) break;
			printf(".");
			//Sleep(sleep);
		}
		printf("%d\r\n",GetLastError());
		closesocket(s);
		Sleep(1000);

	}
	WSACleanup();
}



int _tmain(int argc, _TCHAR* argv[]){
	WSADATA wsaData; int Result;
	if ((Result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){
		printf("WSAStartup failed with error %d\n", Result);
		return 0;
	}

	std::thread th[300];
	//threadfiletrans(200);

	for (int i = 0; i < 300; i++){
		Sleep(500);
		th[i] = std::thread(threadfiletrans,i);
	}

	while (true){ getchar(); }

	return 0;
}

