#pragma once
#ifndef __HDS_FILEINFO_H
#define __HDS_FILEINFO_H

#include "HDS_ns.h"
#include "buffer.h"
#include <string>
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <queue>


NS_XINY120_BEGIN
class cc;
/*
class filetrans{
public:
	filetrans() :fp(0), filesize(0), end(-1), readed(0){};
	~filetrans(){ close(); }
	bool open(const std::string&);
	int32_t getsize();
	int32_t read(char*, const int32_t&);
	int32_t seek(const int32_t&);
	int32_t setend(const int32_t&);
	void close();
	std::string getname(){ return path; };

private:
	FILE* fp;
	int32_t end;
	int32_t readed;
	int32_t filesize;
	std::string path;
};
*/

class speedshot{
public:
	int32_t mbytes;
	uint64_t mtick;
};

typedef std::queue<speedshot> speedqueue;

class filetrans : public OVERLAPPED{
public:
	filetrans(cc* pcc);
	~filetrans(){ close(); if (mbuf != 0) delete[] mbuf; }
	bool open(const std::string&);
	int32_t getsize();
	int32_t read(const int32_t&);
	int32_t seek(const int32_t&);
	int32_t setend(const int32_t&);
	int32_t speed();
	int32_t speedpush(const speedshot&);
	void send();
	void close();
	std::string getname(){ return path; };
	static VOID CALLBACK filereadRoutine(DWORD code, DWORD readed, LPOVERLAPPED ol);

private:
	HANDLE f;
	int32_t end, /*readed,*/ filesize;// , lastout; // 客户端分段下载时指定的结尾。 已经读取了多少。 总文件大小。
	char* mbuf; int32_t mbuflen; const int32_t mbuftotal;
	std::atomic_flag mpendding;// std::recursive_mutex mmutxpendding;
	int32_t mbytestotal; uint64_t mcurtick;
	std::string path;
	cc*	mpcc;
	speedqueue msq;
	
public:
	friend class cc;
};

NS_XINY120_END
#endif