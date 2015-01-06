#pragma once
#ifndef _HDS_BUFFER_H
#define _HDS_BUFFER_H
#include "HDS_ns.h"

#include <winsock2.h>


NS_XINY120_BEGIN

class cc;


enum optype{ acceptex, init, read, write, write2all, close, disconnectex,/* readfile*/ };
class basebuf : public OVERLAPPED {
public:
	basebuf(const optype& _op) :op(_op){ ZeroMemory(this, sizeof(OVERLAPPED)); };
	optype getop(){ return op; };
	WSABUF *  getbuf(){ return (buf); }
protected:
	WSABUF buf[2]; optype op;
};
class acceptexbuf : public basebuf{
public:
	acceptexbuf(SOCKET s) :basebuf(acceptex), msocket(s){};
private:
	SOCKET msocket;
};
class initbuf : public basebuf{ public: initbuf::initbuf() : basebuf(init){}; };
class disconnectexbuf : public basebuf{ public: disconnectexbuf() : basebuf(disconnectex){}; };
class closebuf : public basebuf{ public: closebuf::closebuf() :basebuf(close){}; };
class readbuf : public basebuf{
public:
	readbuf::readbuf() : basebuf(read){};
	bool prepare(cc*);
};
class writebuf : public basebuf{
public:
	writebuf(const char* p, const int32_t& len);
	~writebuf();
};

class write2allbuf : public basebuf{ public: write2allbuf::write2allbuf() : basebuf(write2all){}; void dec(){}; };

//class readfilebuf : public basebuf{ public: readfilebuf(cc* pcc) : basebuf(readfile),mpcc(pcc){}; cc* mpcc; };

NS_XINY120_END

#endif