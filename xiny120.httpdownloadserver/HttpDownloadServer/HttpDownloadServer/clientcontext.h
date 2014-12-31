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
#include <mswsock.h>
#include "fileinfo.h"

NS_XINY120_BEGIN
class cc;

typedef std::map<uint64_t, cc*> mapcc;
typedef std::list<cc*> listcc;

class cc{ // 每连接一个socket只下载一个文件然后断开。
public:
	SOCKET msocket;
	cc(SOCKET);
	~cc();
	int32_t inc();
	int32_t dec();
	bool init();
	inline int32_t getios(){ return mios; };
	inline uint64_t ccid(){ return mccid; };
	bool close();
	int32_t closesocket(bool bG = false);
	inline bool pushchar(const char& ch){ lastline.push_back(ch); return true; };
	inline bool lastlineempty(){ return lastline.empty(); };
	inline void lastlineclear(){ lastline.clear(); };
	inline const std::string& getlastline(){ return lastline; }
	void requestclear();
	bool get();
	std::string varget(const std::string&);
	bool prepareget();
	bool preparereq();
	bool filetrans_push();
	int32_t filetrans_do(const int32_t& count);
	inline std::string getfile(){ return mft.getname(); };
	int32_t getfilesize(){ return mft.getsize(); };
	static void speedreset(){
		cclock lock(cc::ccmutex); mapcc::iterator iter;
		for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
			iter->second->mlastTick = GetTickCount(); iter->second->mtotalreset = 0;
		}
	};
public:
	std::atomic<int64_t> mfirstAct, mlastAct, mioSize, mlastTick, mtotal, mtotalreset, mspeedLimit, muserid;
	char* mbuf; int32_t mbuflen;
	char* mbufw; int32_t mbuflenw;
	bool haverequest, cr, crlf;
	static mapcc cconlines;
	static std::recursive_mutex ccmutex;
	DWORD merrno;
	std::atomic<bool> mftopen, mtouchclose, mspeedlimit;
private:
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