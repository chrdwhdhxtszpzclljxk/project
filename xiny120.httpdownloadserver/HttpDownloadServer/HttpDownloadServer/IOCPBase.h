#pragma once
#ifndef _HDS_IOCPBASE_H_
#define _HDS_IOCPBASE_H_
#include "HDS_ns.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <queue>

#include <WinSock2.h>
#pragma comment( lib, "ws2_32.lib" )
#include <mswsock.h>
#pragma comment(lib,"Mswsock.lib")

NS_XINY120_BEGIN
class cc;
class iocpbase;

const std::string GET = "GET";
const std::string URI = "URI";
const std::string VER = "VER";
const std::string RANGE = "RANGE";
const std::string BYTES = "BYTES";

enum optype{ acceptex, init, read, write, write2all, close, disconnectex};
class basebuf : public OVERLAPPED { 
public:
	basebuf(const optype& _op):op(_op){ ZeroMemory(this, sizeof(OVERLAPPED)); };
	optype getop(){ return op; };
	WSABUF *  getbuf(){	return (buf);	}
protected:
	WSABUF buf[2]; optype op; 
};
class acceptexbuf : public basebuf{
public:
	acceptexbuf(SOCKET s):basebuf(acceptex),msocket(s){};
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

class filetrans{
public:
	filetrans() :fp(0), filesize(0), end(-1), readed(0){};
	~filetrans(){ close(); }
	bool open(const std::string&);
	int32_t getsize();
	int32_t read(char*,const int32_t&);
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
typedef std::lock_guard<std::recursive_mutex> cclock;
typedef std::map<int64_t, int64_t> maplimit;
typedef std::map<uint64_t, cc*> mapcc;
typedef std::list<cc*> listcc;
class cc{ // 每连接一个socket只下载一个文件然后断开。
public:
	SOCKET msocket;
	cc(SOCKET,iocpbase*);
	~cc();
	int32_t inc(){ if(mios > 0) mios++; return mios; };
	int32_t dec();
	inline uint64_t ccid(){ return mccid; };
	void touchclose(iocpbase*);
	inline void setclose(){ mclosetime = time(NULL); };
	inline bool close();
	inline bool pushchar(const char& ch){ lastline.push_back(ch); return true; };
	inline bool lastlineempty(){ return lastline.empty(); };
	inline void lastlineclear(){ lastline.clear(); };
	inline const std::string& getlastline(){ return lastline; }
	inline void requestclear();
	inline bool get();
	inline int32_t getios(){ return mios; };
	std::string varget(const std::string&);
	inline bool prepareget();
	inline bool preparereq();
	inline bool filetrans_push(iocpbase*);
	inline int32_t filetrans_do(const int32_t& count,iocpbase*);
	inline std::string getfile(){ return mft.getname(); };
	int32_t getfilesize(){ return mft.getsize(); };
	bool init();
	static void speedreset(){
		cclock lock(cc::ccmutex); mapcc::iterator iter;
		for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
			iter->second->mlastTick = GetTickCount(); iter->second->mtotalreset = 0;
		}
	};
public:
	std::atomic<int64_t> mfirstAct, mlastAct, mioSize, mlastTick, mtotal,mtotalreset, mspeedLimit, muserid;
	char* mbuf; int32_t mbuflen;
	char* mbufw; int32_t mbuflenw;// , mbuflenw1;
	bool haverequest,cr,crlf;
	static mapcc cconlines;
	static listcc ccdels;
	static std::recursive_mutex ccmutex;
	DWORD merrno;
	std::atomic<bool> mftopen,mtouchclose,mspeedlimit;
private:
	iocpbase* netbase;
	std::atomic<int32_t> mios;
	static std::atomic<uint64_t> __mccid;
	std::atomic<uint64_t> mccid;

	std::string lastline;
	std::map<std::string, std::string> request,varmap;
	filetrans mft;
	int64_t mclosetime;
};

class speedlimit{
public:
	static speedlimit* me();
	void cccame(const int64_t& userid){ if (userid <= 0) return; {cclock lock(mmutex); mlimit[userid] += 1; } cc::speedreset(); };
	void ccgone(const int64_t& userid){ if (userid <= 0) return; {cclock lock(mmutex); mlimit[userid] -= 1; if (mlimit[userid] <= 0) mlimit.erase(userid); }cc::speedreset(); };
	int64_t getcount(){ cclock lock(mmutex); return mlimit.size(); };
	int64_t getcount(const int64_t& userid){ if (userid <= 0) return 0; cclock lock(mmutex); return mlimit[userid]; };
	int64_t getspeed(const int64_t& userid){ if (userid <= 0) return 1; cclock lock(mmutex); return mlimitmax / max(mlimit.size(), 1) / max(mlimit[userid], 1); };
	static std::atomic<int64_t> speedmore;
private:
	speedlimit(){ speedmore = 0; };
	std::recursive_mutex mmutex;
	maplimit mlimit;
	const int64_t mlimitmax = 1024 * 1024 * 8 / 8 * 50;
};


class iocpbase{
public:
	iocpbase();
	virtual ~iocpbase();
	bool start(const int32_t& port);
	bool iocpbase::send(const char* , const int32_t&, cc*);
	void printonlines();

	virtual bool notifyconnection(cc*){ return true; };
	virtual void notifydisconnection(cc*){ /*printf("notifydisconnection\r\n"); */};
	static std::string ltrim(std::string&);
	static std::string rtrim(std::string&);
	static std::string trim(std::string&);

private:
	void threadlistener();
	void threadio();
	void threadfiletrans();
	bool iocpbase::AssociateIncomingClientWithContext(SOCKET clientSocket, in_addr& sa, int32_t& iLen);
	bool iocpbase::ProcessIOMessage(LPOVERLAPPED pBuff, cc *pContext, uint32_t dwSize);
	bool iocpbase::oninit(cc *pContext, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff);
	bool iocpbase::onread(cc* pcc, const int32_t& dwIoSize, const LPOVERLAPPED lpOverlap);
	bool iocpbase::onwrite2all(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff);
	bool iocpbase::onwrite(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff);
	bool iocpbase::aread(cc* pContext);
	void release(LPOVERLAPPED);
	int32_t closesocket(cc *mp, bool bG = false);

	LPFN_DISCONNECTEX mDisConnectEx;
	SOCKET msocketListener;
	HANDLE mcpport, macceptEvent;
	bool msocketInit;
	std::atomic<bool> mshutdown,maccept;
	std::atomic<int32_t> mmaxconnections, mcurconnections;
	int32_t mport;
	std::thread mthreadlistener, mthreadfiletrans; std::vector<std::thread> mthreadios;
};

namespace HttpUtility{
	typedef unsigned char BYTE;
	inline BYTE toHex(const BYTE &x){ return x > 9 ? x - 10 + 'A' : x + '0'; }
	inline BYTE fromHex(const BYTE &x){	return isdigit(x) ? x - '0' : x - 'A' + 10;	}
	inline std::string URLEncode(const std::string &sIn){
		std::string sOut;
		for (size_t ix = 0; ix < sIn.size(); ix++){
			BYTE buf[4]; memset(buf, 0, 4);
			if (isalnum((BYTE)sIn[ix])) { buf[0] = sIn[ix];	}
			//else if ( isspace( (BYTE)sIn[ix] ) ) //貌似把空格编码成%20或者+都可以
			//{
			//    buf[0] = '+';
			//}
			else{
				buf[0] = '%'; buf[1] = toHex((BYTE)sIn[ix] >> 4); buf[2] = toHex((BYTE)sIn[ix] % 16);
			}
			sOut += (char *)buf;
		}
		return sOut;
	};

	inline std::string URLDecode(const std::string &sIn){
		std::string sOut;
		for (size_t ix = 0; ix < sIn.size(); ix++){
			BYTE ch = 0;
			if (sIn[ix] == '%'){
				ch = (fromHex(sIn[ix + 1]) << 4);
				ch |= fromHex(sIn[ix + 2]);
				ix += 2;
			}else if (sIn[ix] == '+') ch = ' ';
			else ch = sIn[ix];
			sOut += (char)ch;
		}
		return sOut;
	}
}

NS_XINY120_END
#endif