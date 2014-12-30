#pragma once
#ifndef __HDS_CLIENTCONTEXT_H
#define __HDS_CLIENTCONTEXT_H
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
#include "fileinfo.h"

NS_XINY120_BEGIN
class cc;

typedef std::lock_guard<std::recursive_mutex> cclock;
typedef std::map<int64_t, int64_t> maplimit;
typedef std::map<uint64_t, cc*> mapcc;
typedef std::list<cc*> listcc;
class cc{ // 每连接一个socket只下载一个文件然后断开。
public:
	SOCKET msocket;
	cc(SOCKET, iocpbase*);
	~cc();
	int32_t inc(){ if (mios > 0) mios++; return mios; };
	int32_t dec();
	inline uint64_t ccid(){ return mccid; };
	//void touchclose(iocpbase*);
	inline void setclose(){ mclosetime = time(NULL); };
	bool close();
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
	inline int32_t filetrans_do(const int32_t& count, iocpbase*);
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
	std::atomic<int64_t> mfirstAct, mlastAct, mioSize, mlastTick, mtotal, mtotalreset, mspeedLimit, muserid;
	char* mbuf; int32_t mbuflen;
	char* mbufw; int32_t mbuflenw;// , mbuflenw1;
	bool haverequest, cr, crlf;
	static mapcc cconlines;
	//static listcc ccdels;
	static std::recursive_mutex ccmutex;
	DWORD merrno;
	std::atomic<bool> mftopen, mtouchclose, mspeedlimit;
private:
	iocpbase* netbase;
	std::atomic<int32_t> mios;
	static std::atomic<uint64_t> __mccid;
	std::atomic<uint64_t> mccid;

	std::string lastline;
	std::map<std::string, std::string> request, varmap;
	filetrans mft;
	int64_t mclosetime;
};

NS_XINY120_END

#endif