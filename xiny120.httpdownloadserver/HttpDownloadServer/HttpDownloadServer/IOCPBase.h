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

#include "buffer.h"

NS_XINY120_BEGIN
class cc;
//class iocpbase;

const std::string GET = "GET";
const std::string URI = "URI";
const std::string VER = "VER";
const std::string RANGE = "RANGE";
const std::string BYTES = "BYTES";

typedef std::map<int64_t, int64_t> maplimit;

class speedlimit{
public:
	DECLARE_SINGLETON_FUNC(speedlimit);
	virtual bool init(){ return true; };
	void cccame(const int64_t& userid);// { if (userid <= 0) return; {cclock lock(mmutex); mlimit[userid] += 1; } cc::speedreset(); };
	void ccgone(const int64_t& userid);// { if (userid <= 0) return; {cclock lock(mmutex); mlimit[userid] -= 1; if (mlimit[userid] <= 0) mlimit.erase(userid); }cc::speedreset(); };
	int64_t getcount(){ cclock lock(mmutex); return mlimit.size(); };
	int64_t getcount(const int64_t& userid){ if (userid <= 0) return 0; cclock lock(mmutex); return mlimit[userid]; };
	int64_t getspeed(const int64_t& userid){ if (userid <= 0) return 1; cclock lock(mmutex); return mlimitmax / max(mlimit.size(), 1) / max(mlimit[userid], 1); };
	std::atomic<int64_t> speedmore;
private:
	speedlimit(){ speedmore = 0; };
	std::recursive_mutex mmutex;
	maplimit mlimit;
	const int64_t mlimitmax = 1024 * 1024 * 8 / 8 * 100;
};


class iocpbase{
public:
	DECLARE_SINGLETON_FUNC(iocpbase);
	virtual bool init(){ return true; };
	bool start(const int32_t& port);
	bool send(const char* , const int32_t&, cc*);
	//bool file2iocp(HANDLE f,cc*);
	void printonlines();

	virtual bool notifyconnection(cc*){ return true; };
	virtual void notifydisconnection(cc*){ /*printf("notifydisconnection\r\n"); */};
	static std::string ltrim(std::string&);
	static std::string rtrim(std::string&);
	static std::string trim(std::string&);
	
	static LPFN_DISCONNECTEX mDisConnectEx;
	static void release(LPOVERLAPPED);
protected:
	iocpbase();
	virtual ~iocpbase();

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
	bool iocpbase::ondisconnectex(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff);
	bool iocpbase::onreadfile(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff);
	bool iocpbase::aread(cc* pContext);

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