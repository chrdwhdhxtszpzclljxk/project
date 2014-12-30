#pragma once
#ifndef __HDS_FILEINFO_H
#define __HDS_FILEINFO_H

#include "HDS_ns.h"
#include <string>
NS_XINY120_BEGIN

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
NS_XINY120_END
#endif