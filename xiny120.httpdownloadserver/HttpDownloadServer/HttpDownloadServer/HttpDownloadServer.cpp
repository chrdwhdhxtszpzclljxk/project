// HttpDownloadServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "IOCPServer.h"
#include <iostream> 
#include <string>
#include "db.h"

xiny120::IOCPServer svr;
std::map<std::string, std::string> cmdmap;
const static char* basepath = "d:\\backup\\";
int _tmain(int argc, _TCHAR* argv[]){
	_setmaxstdio(2048);
	char input[1024] = { 0 }; bool run = true; std::string cmd,par; int32_t len,i,port = 80;
	printf("正准备数据库连接...\r\n");
	FILE* fp = fopen("D:\\sql.txt", "rb");
	if (fp != 0){
		fread(input, 1, sizeof(input), fp);
		fclose(fp);
	}else{
		printf("打开数据库配置文件失败！\r\n"); getchar(); return 0;
	}
	printf("%s\r\n",input);
	xiny120::db::setupdb(input);
	xiny120::db::me();
	printf("数据库连接准备完毕！准备监听端口...\r\n");
	if(svr.start(port))	printf("服务器启动于端口：%d\r\n",port);
	else printf("监听 %d 端口失败\r\n",port);
	do{
		std::cin.getline(input, 1023);
		len = strlen(input); cmd.clear(); par.clear();
		for (i = 0; i < len; i++){
			if (input[i] == ' ' || input[i] == '\t') break;
			cmd.push_back(input[i]);
		}
		if (!cmd.empty()){
			par.append(input + i,input + len);
			if (cmd == "cls") system("cls");
			else if (cmd == "c") svr.printonlines();
		}
		//if (strcmp(cmd, "cls") == 0) system("cls");
		//else if (strcmp() == 0)
	} while (run);
	
	return 0;
}

const char* getbasepath(){ return basepath; };
