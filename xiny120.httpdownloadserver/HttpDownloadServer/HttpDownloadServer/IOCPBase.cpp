#include "IOCPBase.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "db.h"
#include "output.h"

#pragma warning(disable:4996)
extern const char* getbasepath();
NS_XINY120_BEGIN

static speedlimit* __speedlimit = 0;
std::atomic<int64_t> speedlimit::speedmore = 0;
speedlimit* speedlimit::me(){
	if (__speedlimit == 0){ __speedlimit = new speedlimit();}
	return __speedlimit;
}




void iocpbase::threadfiletrans(){
	mapcc::iterator iter; int64_t last = 0, cur = 0, cccount = 0, speedmore = 0; int32_t out = 0;
	while (!mshutdown){
		{ cclock lock(cc::ccmutex); cccount = cc::cconlines.size(); }
		{
			/*
			cclock lock(cc::ccmutex);
			if (cc::ccdels.size() > 0){
				listcc::iterator it1;
				for (listcc::iterator it = cc::ccdels.begin(); it != cc::ccdels.end(); it++){
					cc* obj = (*it);
					obj->mftopen = false;
					if (obj->close()){
						cc::cconlines.erase((*it)->ccid()); delete obj;
						it1 = it; it1++;
						cc::ccdels.erase(it);
						if (it1 == cc::ccdels.end()) break;
						it = it1;
						//printf("close??\r\n");
					}
				}
				//cc::ccdels.clear();
			}
			*/
		}

		if (cccount <= 0) { SleepEx(100, TRUE); continue; } // 在线人数为0，线程休息。
		cur = GetTickCount();
		if ((cur - last) >= 100){ // 距离上次不足100，跳过。
			cclock lock(cc::ccmutex);
			for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
				if (!iter->second->mftopen){ iter->second->close(); continue; }
				int64_t speed = speedlimit::me()->getspeed(iter->second->muserid) / 10;
				int64_t speeded = 0;
				//Sleep(1);
				if (iter->second->getios() < 10){
					while (speed > 0 && (iter->second->getios() < 5)){
						out = iter->second->filetrans_do(speed, this);
						if (out == 0) break;
						speeded += out; if (speeded >= speed) break;
						speed -= out;
						//printf("%d - %d\r\n", int32_t(speed), int32_t(GetTickCount() - last));
					}
					while (speedmore > 0 && (iter->second->getios() < 3)){
						out = iter->second->filetrans_do(speedmore, this);
						if (out == 0) break;
						speeded += out; if (speeded >= speed) break;
						speedmore -= out;
						//printf("%d - %d\r\n", int32_t(speedmore), int32_t(GetTickCount() - last));
					}
				} else	speedmore += speed; // 带宽太小，推数据时被阻塞，所以跳过发送并把带宽贡献给后面的高速用户。
			}
			last = cur;
		}
		if (speedmore >= 1024 * 1024 * 3 / 2){ speedmore = 1024 * 1024 * 2 / 3; Sleep(10); }


		Sleep(1);
	}
}



iocpbase::iocpbase() : msocketInit(false){
	WSADATA wsaData; int Result;
	if ((Result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){
		printf("WSAStartup failed with error %d\n", Result);
		return;
	}
	msocketInit = true; mmaxconnections = 100000;
}

iocpbase::~iocpbase(){ if (msocketInit) WSACleanup();}

int32_t iocpbase::closesocket(cc* pcc, bool bG){
	int32_t ret = 0, dwError = 0;
	if (bG && mDisConnectEx != NULL){
		if (pcc->inc() > 0){
			disconnectexbuf*  pOverlappedBuffer = new disconnectexbuf();
			BOOL bRet = mDisConnectEx(pcc->msocket, pOverlappedBuffer, 0, 0);
			if (bRet || ((dwError = WSAGetLastError()) == ERROR_IO_PENDING)){
				bRet = TRUE;
				//_tprintf(_T("m_pDisConnectEx：%d %d\r\n"),dwError,bRet);
				//_tprintf(_T("m_pDisConnectEx：%d PENDING：%d OK!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
				return true;
			}else{
				//_tprintf(_T("m_pDisConnectEx：%d PENDING：%d FAIL!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
			}
			//printf("\nCloseSocket\n");
			pcc->dec();
			release(pOverlappedBuffer);
		}
	}
	if (pcc->msocket != INVALID_SOCKET){ ret = ::closesocket(pcc->msocket); pcc->msocket = INVALID_SOCKET; }
	else{
		//_tprintf(_T("\r\n\r\nsocket关闭了。但是没有排除出列表？ %s \t %d\r\n"),mp->m_ci.m_szUserName,mp->m_Socket);
	}
		//_tprintf(_T("\n不太可能的错误吧？socket关闭了。但是没有排除出列表？ %s CCID：%I64u \t PENDING：%d\r\n"),mp->m_ci.m_tcUserName,mp->m_u64CCID,mp->m_nNumberOfPendlingIO);
	return ret;
}

void iocpbase::threadio(){
	BOOL bError = FALSE, bIORet = FALSE; DWORD dwIoSize = 0, dwIOError = 0;
	cc*	pcc = NULL; LPOVERLAPPED lpOverlapped = NULL; ULONG_PTR	ulOut = 0;
	CoInitialize(NULL);	
	while (!bError){
		pcc = NULL; lpOverlapped = NULL; dwIoSize = 0;
		bIORet = GetQueuedCompletionStatus(mcpport, &dwIoSize, &ulOut, &lpOverlapped, INFINITE);
		pcc = (cc*)ulOut;
		if (bIORet){//ok
			if (lpOverlapped && pcc){
				if (!ProcessIOMessage(lpOverlapped, pcc, dwIoSize)) __noop;// pcc->dec();
			} else if ((pcc == NULL) && (lpOverlapped == NULL) && mshutdown){
				bError = TRUE;	// 这里表示结束工作者线程。
				continue;
			}else{
				assert(0);
			}
		}else{// If Something whent wrong..
			pcc->merrno = dwIOError = GetLastError();
			if (dwIOError != WAIT_TIMEOUT){ // It was not an Time out event we wait for ever (INFINITE) 
				if (pcc != NULL){
					/*
					* ERROR_NETNAME_DELETED Happens when the communication socket
					* is cancelled and you have pendling WSASend/WSARead that are not finished.
					* Then the Pendling I/O (WSASend/WSARead etc..) is cancelled and we return with
					* ERROR_NETNAME_DELETED..
					*/
					if (dwIOError == ERROR_NETNAME_DELETED){
					}else{ // Should not get here if we do: disconnect the client and cleanup & report. 
						//wchar_t szError[1024] = { 0 };
						//swprintf_s(szError, _countof(szError), L"不可能的错误：%d 用户：%s\r\n", dwIOError, lpClientContext->m_ci.m_szUserName);
						//OutputDebugStringW(szError);
						//assert(0);
					}
				}else{
					// We shall never come here  
					// anyway this was an error and we should exit the worker thread
					assert(0);
					bError = TRUE;
				}

			}	// if(dwIOError != WAIT_TIMEOUT) 
			else
			{
				// 不可能运行到这里！！！
				assert(0);
			}
		} // if (!bIORet) 
		basebuf* p = (basebuf*)lpOverlapped;
		//printf("IOWorkerThreadProc bIORet：%d\t Operater：%d\r\n",bIORet,p->GetOperation());
		if(pcc != NULL) pcc->dec();	// IO返回了，不管是否成功，都需要把未决IO减去1
		release(lpOverlapped);
	}
	CoUninitialize();
}


void iocpbase::threadlistener(){
	BOOL bSuccessed = FALSE; mshutdown = false; WSANETWORKEVENTS events;
	while (!mshutdown){
		if (WSAWaitForMultipleEvents(1, &macceptEvent, FALSE, 100, FALSE) == WSA_WAIT_TIMEOUT) continue;
		if (WSAEnumNetworkEvents(msocketListener, macceptEvent, &events) == SOCKET_ERROR) continue; // 可能需要重启。
		bSuccessed = FALSE; char* p = "null";
		if (events.lNetworkEvents & FD_ACCEPT){
			if (events.iErrorCode[FD_ACCEPT_BIT] == 0 && maccept && !mshutdown){
				SOCKET	clientSocket = INVALID_SOCKET; int32_t	nRet = -1, nLen = sizeof(SOCKADDR_IN);
				if (INVALID_SOCKET != (clientSocket = WSAAccept(msocketListener, NULL, &nLen, 0, 0))){
					const char chOpt = 1;
					if (SOCKET_ERROR != setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char))){		// 关闭合并小块算法。
						in_addr sa = { 0 };	int32_t	iLen = 0;
						AssociateIncomingClientWithContext(clientSocket, sa, iLen);
						p = inet_ntoa(sa);
						bSuccessed = TRUE;
					}else ::closesocket(clientSocket);
				}
			}
		}
		cclock lock(cc::ccmutex);
		//printf(("用户：%s[%d] Accept Status：%d\r\n"), p, cc::cconlines.size(), bSuccessed);
	}
}

bool iocpbase::AssociateIncomingClientWithContext(SOCKET clientSocket, in_addr& sa, int32_t& iLen){
	if (mshutdown || (!maccept) || (clientSocket == INVALID_SOCKET)) return false;
	if (mcurconnections >= mmaxconnections){// Close connecttion if we have reached the maximum nr of connections... 
		LINGER li; li.l_onoff = 1; li.l_linger = 0;
		setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li));
		::closesocket(clientSocket); clientSocket = INVALID_SOCKET;
		return false;
	}
	cc* pcc = new cc(clientSocket,this);
	pcc->init();
	getpeername(clientSocket, (SOCKADDR*)&sa, &iLen);// 获取一下客户IP地址，并记录
	if (CreateIoCompletionPort((HANDLE)clientSocket, mcpport, (ULONG_PTR)pcc, 0) == mcpport){
		// Trigger first IO Completion Request
		// Otherwise the Worker thread will remain blocked waiting for GetQueuedCompletionStatus...
		// The first message that gets queued up is ClientIoInitializing - see ThreadPoolFunc and 
		// IO_MESSAGE_HANDLER
		// Important!! EnterIOLoop must notify that the socket and the structure  
		// pContext have an Pendling IO operation ant should not be deleted.
		// This is nessesary to avoid Access violation. 
		initbuf* pinit = new initbuf();//AllocateBuffer(IOInitialize);
		if (pinit != NULL){
			BOOL bSuccess = PostQueuedCompletionStatus(mcpport, 0, (ULONG_PTR)pcc, pinit);
			if (bSuccess || (GetLastError() == ERROR_IO_PENDING)) return true;
			delete pinit;
		}
	}
	delete pcc; // IO投递没成功，不能经过 线程 IOWorkerThreadProc 释放IO，需要手工释放。
	return false;
}


bool iocpbase::start(const int32_t& port){
	mport = port; int32_t err;
	mcpport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL != mcpport){ // 创建IO内核对象失败  
		if (INVALID_SOCKET != (msocketListener = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED))){
			GUID GuidAcceptEx = WSAID_DISCONNECTEX; DWORD dwBytes = 0;
			// Get AccpetEx Function
			if (WSAIoctl(msocketListener, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx),
				&mDisConnectEx, sizeof(mDisConnectEx), &dwBytes, NULL, NULL) == 0){

				if (WSA_INVALID_EVENT != (macceptEvent = WSACreateEvent())){ // Event for handling Network IO
					// The listener is ONLY interested in FD_ACCEPT
					// That is when a client connects to or IP/Port
					// Request async notification
					if (WSAEventSelect(msocketListener, macceptEvent, FD_ACCEPT) == 0){
						SOCKADDR_IN		saServer;
						saServer.sin_port = htons(mport);// Listen on our designated Port#
						saServer.sin_family = AF_INET;// Fill in the rest of the address structure
						saServer.sin_addr.s_addr = INADDR_ANY;
						if ((err = bind(msocketListener, (LPSOCKADDR)&saServer, sizeof(struct sockaddr))) != SOCKET_ERROR){// bind our name to the socket
							if (listen(msocketListener, 500) != SOCKET_ERROR){// Set the socket to listen
								maccept = true;
								mthreadlistener = std::thread(&iocpbase::threadlistener, this);
								for (int32_t i = 0; i < 8; i++) mthreadios.push_back(std::thread(&iocpbase::threadio, this));
								mthreadfiletrans = std::thread(&iocpbase::threadfiletrans, this);
								return true;
							}
							else{ err = WSAGetLastError(); }
						}
						else{ err = WSAGetLastError(); }
					}
					else{ err = WSAGetLastError(); }
					WSACloseEvent(macceptEvent);
				}
			}
			::closesocket(msocketListener);
		}
	}
	return false;
}

void iocpbase::release(LPOVERLAPPED pbuf){
	if (pbuf == NULL) return;
	basebuf* pbase = (basebuf*)pbuf;
	switch (pbase->getop()){
	case init: delete (initbuf*)pbuf;break;
	case read: delete (readbuf*)pbuf;break;
	case write: delete (writebuf*)pbuf;	break;
	case write2all:{
		write2allbuf* pall = (write2allbuf*)pbuf;
		pall->dec();
	}break;
	default:
		assert(0);
		break;
	}
}

bool iocpbase::ProcessIOMessage(LPOVERLAPPED pBuff, cc *pContext, uint32_t dwSize){
	if (pBuff == NULL) return false;
	bool bRet = false; int32_t iError = 0; basebuf* pbase = (basebuf *)pBuff;
	switch (pbase->getop()){
	case init: bRet = oninit(pContext, dwSize, pBuff); iError = 1;  break;
	case read: bRet = onread(pContext, dwSize, pBuff); iError = 2;	break;
	case write2all: bRet = onwrite2all(pContext, dwSize, pBuff); iError = 3; break;
	case write:	bRet = onwrite(pContext, dwSize, pBuff); iError = 4; break;
	case disconnectex: bRet = ondisconnectex(pContext, dwSize, pBuff); iError = 4; break;
	default: pContext->merrno = 999; iError = 99; break;
	}
	return bRet;
}

bool iocpbase::oninit(cc *pContext, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff){
	if(notifyconnection(pContext)) return aread(pContext);
	return false;
}

bool iocpbase::ondisconnectex(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff){
	closesocket(pcc);
	delete pcc;
	return true;
}

// The read is not made directly to distribute CPU power fairly between the connections.
bool iocpbase::aread(cc* pcc){ // 这里可以用客户端那个组合缓冲哦。。避免了copy了。有待改进。
	bool ret = false; uint32_t nRetVal = 0, dwIoSize = 0, wsaError = 0, ulFlags = MSG_PARTIAL;
	LPWSABUF lpwsaBuf = NULL; readbuf* pread = NULL; BOOL bDeleted = FALSE;
	if (pcc != NULL){ // 发起一个异步读socket操作
		if(NULL != (pread = new readbuf())){
			pread->prepare(pcc); lpwsaBuf = pread->getbuf();
			if (pcc->inc() > 0){
				nRetVal = WSARecv(pcc->msocket, lpwsaBuf, 1, (LPDWORD)&dwIoSize, (LPDWORD)&ulFlags, pread, NULL);
				if (nRetVal != SOCKET_ERROR || (wsaError = WSAGetLastError()) == WSA_IO_PENDING) return true;
				else pcc->merrno = wsaError;
			}
			release(pread);
		}
		pcc->dec();// IO投递没成功，不能经过 线程 IOWorkerThreadProc 释放IO，需要手工释放。
	}
	return false;
}

bool iocpbase::send(const char* p, const int32_t& len, cc* pcc){
	LPWSABUF pbuf = NULL; uint32_t nRetVal = 0, dwIoSize = 0, ulFlags = MSG_PARTIAL; int32_t wsaError = 0, iOnline = 0;
	if (pcc != NULL && pcc->msocket != INVALID_SOCKET){
		if (pcc->inc() > 0){	// 发起一个IO操作之前，先增加IO数
			writebuf* wb = new writebuf(p, len);
			pbuf = wb->getbuf();
			if (len != pbuf->len) assert(0);
			//printf("%d - %d\r\n",len, pbuf->len);
			nRetVal = WSASend(pcc->msocket, pbuf, 1, (LPDWORD)&dwIoSize, ulFlags, wb, NULL);
			if (nRetVal != SOCKET_ERROR || (wsaError = WSAGetLastError()) == WSA_IO_PENDING){
				return true;
			}else{
				pcc->dec(); // IO投递没成功，不能经过 线程 IOWorkerThreadProc 释放IO，需要手工释放。
				release(wb);
			}
		}
	}
	return false;
}


bool iocpbase::onread(cc* pcc, const int32_t& len, const LPOVERLAPPED lpOverlap){
	bool ret = false; readbuf* pread = (readbuf*)lpOverlap;	int32_t i = 0, idx = 0; char path[1024] = { 0 };
	if (len == 0 || pread == NULL){ pcc->merrno = 999; return false; }// 有可能客户段关闭了 socket
	if (pcc != NULL){
		// 保持每个 ClientContext 只投递一个 读取 申请，这样的话， ClientContext 组包的上下文就不用同步了。
		// 实际上 一次投递多个 读取包也没有必要，如果计算繁重，数据量也繁重！可以加大读取包的长度，而没有必要
		// 投递多个读取 包。投递多个读取 包 还会造成包的顺序混乱，还需要对包进行排续，所以没有必要投递多个读取包
		// 另外 投递时的 LPWSABUF 也支持一次有序的投递多个包（是一个用 WSARecv投递多个LPWSABUF读取缓冲，而且返回时也是有序的，
		// 而不用 使用 WSARecv 每次投递一个 LPWSABUF 来进行多次投递读请求）。
		for (i = 0; i < len; i++){
			if (pcc->mbuf[i] == 0x0d){ pcc->cr = true; i++; if (i >= len) break; }
			if (pcc->cr && pcc->mbuf[i] == 0x0a){ // 探测的回车换行。
				if (pcc->crlf){ // 前面已经有过回车换行了。双回车换行表示请求结束，开始处理请求。
					pcc->haverequest = true; //printf("请求结束！开始下发文件。\r\n");
					ret = pcc->filetrans_push(this);
					if (!ret) return ret;
				} else { // 单回车
					pcc->crlf = true; if (i >= len){ assert(0); break; }
					if (!pcc->lastlineempty()){ // 遇到回车换行时，当前行不为空，就证明是有效数据！
						if (!pcc->get()){ // 第一行必须是get
							if (!pcc->prepareget()) return false; // 如果第一行不是get，就返回错误。
							//printf("%s\r\n", pcc->getlastline().c_str());
						}else{
							//printf("%s\r\n",pcc->getlastline().c_str());
							pcc->preparereq();
						}

						pcc->lastlineclear();
						idx++;
					}
				}
			} else { 
				pcc->crlf = pcc->cr = false;
				if (pcc->haverequest) { printf("一次连接只能下载一个文件。如果有多余请求，关闭连接。\r\n"); return false; }// 一次连接只能下载一个文件。如果有多余请求，关闭连接。
				pcc->pushchar(pcc->mbuf[i]); 
			};
		}
		ret = aread(pcc);
	}
	return ret;
}

bool iocpbase::onwrite2all(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff){
	if (pcc != NULL) pcc->mlastAct = time(NULL);
	return true;
}

bool iocpbase::onwrite(cc *pcc, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff){
	if (pcc != NULL) { pcc->mlastAct = time(NULL); }
	if (dwIoSize == 0){ if (pcc->mtouchclose) { /*otprint("[用户%d]关闭连接!\r\n", pcc->muserid);*/ pcc->setclose(); return false; }; }
	pcc->mtotal += dwIoSize; pcc->mtotalreset += dwIoSize;
	return true;
}



void iocpbase::printonlines(){
	cclock lock(cc::ccmutex); mapcc::iterator iter; std::string out, line; std::stringstream ss; int32_t i = 0; int64_t span;
	double speed;
	ss << "在线数：" << cc::cconlines.size();
	ss >> line; ss.clear(); out.append(line).append("\r\n");
	for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
		ss << std::setfill('0') << std::setw(6) << i << "[" << iter->first << "][用户" << std::setfill('0') << std::setw(10)<< iter->second->muserid << "][" << iter->second->getfile() << "][活动";
		ss << iter->second->mlastAct - iter->second->mfirstAct << "][完成" << iter->second->mioSize << "(" << std::fixed << std::setprecision(2) << (double(iter->second->mioSize * 100) / double(iter->second->getfilesize())) << "%%)][速度";
		span = (GetTickCount() - iter->second->mlastTick) / 1000;
		if (span < 1) span = 1;
		speed = double(iter->second->mtotalreset) / double(span) / 1024.0f;
		ss << std::fixed << std::setprecision(2) << speed << "k/s][" << speedlimit::me()->getspeed(iter->second->muserid) << "]";
		i++; ss >> line; ss.clear(); out.append(line).append("\r\n");
	}
	printf(out.c_str());
}

std::string iocpbase::ltrim(std::string& str){
	str.erase(str.begin(), std::find_if(str.begin(), str.end(),
		std::not1(std::ptr_fun(::isspace))));
	return str;
}

std::string iocpbase::rtrim(std::string& str){
	str.erase(std::find_if(str.rbegin(), str.rend(),
		std::not1(std::ptr_fun(::isspace))).base(),
		str.end());
	return str;
}

std::string iocpbase::trim(std::string& str){
	return rtrim(ltrim(str));
}


NS_XINY120_END