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

bool readbuf::prepare(cc* pcc){ buf[0].buf = pcc->mbuf; buf[0].len = pcc->mbuflen; return true;}

writebuf::writebuf(const char* p, const int32_t& len) : basebuf(write){ 
	try{ buf[0].len = len; buf[0].buf = new char[len]; memcpy(buf[0].buf, p, len);}catch (...){ assert(0);}
}

writebuf::~writebuf(){ delete [] buf[0].buf;}

bool filetrans::open(const std::string& fn){
	if (fp != 0) fclose(fp); path = fn;
	if ((fp = fopen(fn.c_str(), "rb")) == 0) return false;
	if (fp != 0){ fseek(fp, 0, SEEK_END); filesize = ftell(fp); fseek(fp, 0, SEEK_SET); }
	end = -1; readed = 0;
	return true;
}

void filetrans::close(){ if (fp != 0) fclose(fp); fp = 0; }
int32_t filetrans::setend(const int32_t& _end){ end = _end; return end; }
int32_t filetrans::seek(const int32_t& pos){ int32_t len; if (fp != 0){ len = fseek(fp, pos, SEEK_SET); readed += len; return len; } return -1; };
int32_t filetrans::read(char* buf, const int32_t& _len){ 
	int32_t len = _len; 
	if (fp != 0){
		if (end != -1){ // range ָ���˽�����ǡ�
			if (readed >= end) return 0; // �Ѿ���ȡ���ֽڴ��ڵ���rangeָ�����ֽڣ�ֹͣ��ȡ��
			if ((readed + len) > end){
				len = end - readed;
			}
		}
		len = fread(buf, 1, len, fp); 
		readed += len; 
		return len; 
	} 
	return 0; 
}

int32_t filetrans::getsize(){ return filesize;}

mapcc cc::cconlines;
std::recursive_mutex cc::ccmutex;
cc::cc(SOCKET socket, iocpbase* b) : msocket(socket), netbase(b), mios(1), mbuflen(2048), mbuflenw(1024 * 32), mclosetime(-1),
mftopen(false), mbuf(0), mbufw(0), mtouchclose(false), mtotal(0), mtotalreset(0), mspeedLimit(1024 * 100), mspeedlimit(false),
haverequest(false), cr(false), crlf(false), mfirstAct(time(NULL)), mlastAct(mfirstAct + 1), mlastTick(0), mioSize(0){
	cclock lock(ccmutex); cconlines[msocket] = this; 
}
cc::~cc(){ 
	if (mspeedlimit) speedlimit::me()->ccgone(muserid);
	{cclock lock(ccmutex); cconlines.erase(msocket); }
	closesocket(msocket);
	netbase->notifydisconnection(this);
	if (mbuf != 0) delete[] mbuf;
	if (mbufw != 0) delete[] mbufw;
};

bool cc::init(){ mbuf = new char[mbuflen]; mbufw = new char[mbuflenw]; return true;}
int32_t cc::dec(){ if (--mios <= 0){ delete this; return 0; } return mios;}
void cc::touchclose(iocpbase* psvr){ mtouchclose = true; psvr->send("", 0, this);}

inline void cc::close() { 
	//printf("�ر��ļ�\r\n"); 
	if (mclosetime <= 0){
		if ((time(NULL) - mlastAct) > 60) shutdown(msocket, 2);
	} 
	if ((time(NULL) - mclosetime) > 10)
		shutdown(msocket, 2);//closesocket(msocket);
};

void cc::requestclear(){
	std::map<std::string, std::string>::iterator iter;
	for (iter = request.begin(); iter != request.end(); iter++){ iter->second.clear();}
	lastlineclear();
	haverequest = false;
}

inline bool cc::get(){
	std::map<std::string, std::string>::iterator iter = request.find(GET); 
	if (iter != request.end()) return true;
	return false;
}

std::string cc::varget(const std::string& key){
	return varmap[key];
}

inline bool cc::prepareget(){
	int32_t i = 0, len = 0; std::stringstream ss(lastline); std::string txt, var, val;
	ss >> txt; std::transform(txt.begin(), txt.end(), txt.begin(), ::toupper);
	if (txt != GET) return false;
	request[GET] = GET;
	ss >> txt; txt = HttpUtility::URLDecode(txt); if (!txt.empty()) request[URI] = txt; len = txt.length();
	for (i = 0; i < len; i++){
		if (txt[i] == '='){	var = val; val.clear();	} 
		else if (txt[i] == '&') { varmap[var] = val; var.clear(); val.clear(); } 
		else if (txt[i] == '?') { request[GET] = val; val.clear(); var.clear(); }
		else val.push_back(txt[i]);
	}
	if ((!var.empty()) && (!val.empty())) varmap[var] = val;
	ss >> txt; if (!txt.empty()) request[VER] = txt;
	return true;
}

inline bool cc::preparereq(){
	int32_t i = 0, len = 0; std::string txt = lastline, var, val; len = txt.length();
	for (i = 0; i < len; i++){
		if (txt[i] == ':'){ var = val; val.clear(); }
		else val.push_back(txt[i]);
	}
	std::transform(var.begin(), var.end(), var.begin(), ::toupper);
	if ((!var.empty()) && (!val.empty()) && var == RANGE) request[var] = val;
	return true;
}

inline bool cc::filetrans_push(iocpbase* psvr){
	mft.close(); std::string file; char buf[1024] = { 0 }; char filename[1024] = { 0 }; char ext[256] = { 0 }; const char* basepath = getbasepath();
	int32_t userid = 0, i = 0, len = 0, cl,s,e; bool ret = false; mftopen = false; std::string txt, var, val,resstr = "HTTP/1.1 200 OK" ; std::stringstream ss;
	char cr[1024] = { 0 };
	std::string key = varget("key"); if (!key.empty()){
		file = db::me()->getfile(key, userid); if (!(file.empty() || userid == 0)){
			muserid = userid; _splitpath(file.c_str(), NULL, NULL, filename, ext);
			if (!mft.open(file)){
				otprint("[%d][%s]�ļ�������(%s)\r\n",muserid,key.c_str(), file.c_str()); return false;
			}
			else {
				cl = mft.getsize();
				if (!request[RANGE].empty()){ // ������range������Ҫ���з�����
					txt = request[RANGE]; len = txt.length();
					for (i = 0; i < len; i++){
						if (txt[i] == '='){ var = val; val.clear(); }
						else val.push_back(txt[i]);
					}
					std::transform(var.begin(), var.end(), var.begin(), ::toupper);
					var = iocpbase::trim(var);
					if (var == BYTES){
						s = e = 0;
						txt = val; val.clear(); len = txt.length();
						for (i = 0; i < len; i++){
							if (txt[i] == '-'){ var = val; val.clear(); }
							else val.push_back(txt[i]);
						}
						if (!var.empty()){
							ss.clear(); ss << var; ss >> s; if (mft.seek(s) == -1) return false;
						}
						if (!val.empty()){
							ss.clear(); ss << val; ss >> e; mft.setend(e);
						}
						resstr = "HTTP/1.1 206 Partial Content";
						sprintf(cr, "Content-Range:bytes %d-%d/%d\r\n", s, e, cl);
						cl = e - s + 1;
					}
				}
				sprintf(buf, "%s\r\n" //http://blog.csdn.net/pud_zha/article/details/7924929
					"Content-Length:%d\r\n"
					"Content-Type:application/octet-stream\r\n"
					"Accept-Ranges:bytes\r\n"
					"%s"
					"Server:httpdownloadserver\r\n"
					"Connection:close\r\n"
					"Content-Disposition:attachment;filename=%s%s\r\n\r\n", resstr.c_str(),cl, cr, filename, ext);
				mspeedlimit = true;
				speedlimit::me()->cccame(muserid);
				psvr->send(buf, strlen(buf), this);
				mftopen = true;
				mlastTick = GetTickCount();
				//otprint("[%d][%s]��ʼ���أ�%s\r\n",int32_t(muserid),key.c_str(),file.c_str());
				return true;
			}
		}
		else{
			ret = false;
		}
	}

	if (!ret){
		sprintf(buf, "HTTP/1.0 403 Forbidden\r\n"
			"Content-Type:text/plain\r\n"
			"Server:httpdownloadserver\r\n"
			"Connection:close\r\n\r\nerror:403\r\n\r\n");
		psvr->send(buf, strlen(buf), this);
		touchclose(psvr);
	}

	return false;
}

inline int32_t cc::filetrans_do(const int32_t& _count,iocpbase* psvr){
	if (!mftopen){ /*otprint("filetrans_do �����ļ�û�򿪣�\r\n");*/return true; }
	int32_t count = _count; if (count > mbuflenw) count = mbuflenw;
	count = mft.read(mbufw, count);
	if (count > 0){ psvr->send(mbufw, count, this); mioSize += count; mlastAct = time(NULL); }
	else if(count == 0){ // ����ȫ���ļ���ɣ�����һ�����Źر�֪ͨ�����ӷ����б��������
		mftopen = false; mft.close();
		psvr->send("\r\n", 2, this);
		touchclose(psvr);
	};
	return count;
}

void iocpbase::threadfiletrans(){
	mapcc::iterator iter; int64_t last = 0, cur = 0, cccount = 0, speedmore = 0; int32_t out = 0;
	while (!mshutdown){
		{ cclock lock(cc::ccmutex); cccount = cc::cconlines.size(); }
		if (cccount <= 0) { SleepEx(100, TRUE); continue; } // ��������Ϊ0���߳���Ϣ��
		cur = GetTickCount();
		if ((cur - last) >= 100){ // �����ϴβ���100��������
			cclock lock(cc::ccmutex);
			for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
				if (!iter->second->mftopen){ iter->second->close(); continue; }
				int64_t speed = speedlimit::me()->getspeed(iter->second->muserid) / 10;
				int64_t speeded = 0;

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
				} else	speedmore += speed; // ����̫С��������ʱ�������������������Ͳ��Ѵ����׸�����ĸ����û���
			}
			last = cur;
		}
		if (speedmore >= 1024 * 1024 * 3 / 2){ speedmore = 1024 * 1024 * 2 / 3; Sleep(50); }
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
				bError = TRUE;	// �����ʾ�����������̡߳�
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
						//swprintf_s(szError, _countof(szError), L"�����ܵĴ���%d �û���%s\r\n", dwIOError, lpClientContext->m_ci.m_szUserName);
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
				// ���������е��������
				assert(0);
			}
		} // if (!bIORet) 
		basebuf* p = (basebuf*)lpOverlapped;
		//printf("IOWorkerThreadProc bIORet��%d\t Operater��%d\r\n",bIORet,p->GetOperation());
		if(pcc != NULL) pcc->dec();	// IO�����ˣ������Ƿ�ɹ�������Ҫ��δ��IO��ȥ1
		release(lpOverlapped);
	}
	CoUninitialize();
}


void iocpbase::threadlistener(){
	BOOL bSuccessed = FALSE; mshutdown = false; WSANETWORKEVENTS events;
	while (!mshutdown){
		if (WSAWaitForMultipleEvents(1, &macceptEvent, FALSE, 100, FALSE) == WSA_WAIT_TIMEOUT) continue;
		if (WSAEnumNetworkEvents(msocketListener, macceptEvent, &events) == SOCKET_ERROR) continue; // ������Ҫ������
		bSuccessed = FALSE; char* p = "null";
		if (events.lNetworkEvents & FD_ACCEPT){
			if (events.iErrorCode[FD_ACCEPT_BIT] == 0 && maccept && !mshutdown){
				SOCKET	clientSocket = INVALID_SOCKET; int32_t	nRet = -1, nLen = sizeof(SOCKADDR_IN);
				if (INVALID_SOCKET != (clientSocket = WSAAccept(msocketListener, NULL, &nLen, 0, 0))){
					const char chOpt = 1;
					if (SOCKET_ERROR != setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char))){		// �رպϲ�С���㷨��
						in_addr sa = { 0 };	int32_t	iLen = 0;
						AssociateIncomingClientWithContext(clientSocket, sa, iLen);
						p = inet_ntoa(sa);
						bSuccessed = TRUE;
					}else closesocket(clientSocket);
				}
			}
		}
		cclock lock(cc::ccmutex);
		//printf(("�û���%s[%d] Accept Status��%d\r\n"), p, cc::cconlines.size(), bSuccessed);
	}
}

bool iocpbase::AssociateIncomingClientWithContext(SOCKET clientSocket, in_addr& sa, int32_t& iLen){
	if (mshutdown || (!maccept) || (clientSocket == INVALID_SOCKET)) return false;
	if (mcurconnections >= mmaxconnections){// Close connecttion if we have reached the maximum nr of connections... 
		LINGER li; li.l_onoff = 1; li.l_linger = 0;
		setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li));
		closesocket(clientSocket); clientSocket = INVALID_SOCKET;
		return false;
	}
	cc* pcc = new cc(clientSocket,this);
	pcc->init();
	getpeername(clientSocket, (SOCKADDR*)&sa, &iLen);// ��ȡһ�¿ͻ�IP��ַ������¼
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
	delete pcc; // IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
	return false;
}


bool iocpbase::start(const int32_t& port){
	mport = port; int32_t err;
	mcpport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL != mcpport){ // ����IO�ں˶���ʧ��  
		if (INVALID_SOCKET != (msocketListener = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED))){
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
						if (listen(msocketListener, 5) != SOCKET_ERROR){// Set the socket to listen
							maccept = true;
							mthreadlistener = std::thread(&iocpbase::threadlistener, this);
							for (int32_t i = 0; i < 8; i++) mthreadios.push_back(std::thread(&iocpbase::threadio, this));
							mthreadfiletrans = std::thread(&iocpbase::threadfiletrans, this);
							return true;
						}else{ err = WSAGetLastError(); }
					}else{ err = WSAGetLastError(); }
				}else{ err = WSAGetLastError(); }
				WSACloseEvent(macceptEvent);
			}
			closesocket(msocketListener);
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
	if (pBuff == NULL) return FALSE;
	bool bRet = FALSE; int32_t iError = 0; basebuf* pbase = (basebuf *)pBuff;
	switch (pbase->getop()){
	case init: bRet = oninit(pContext, dwSize, pBuff); iError = 1;  break;
	case read: bRet = onread(pContext, dwSize, pBuff); iError = 2;	break;
	case write2all: bRet = onwrite2all(pContext, dwSize, pBuff); iError = 3; break;
	case write:	bRet = onwrite(pContext, dwSize, pBuff); iError = 4; break;
	default: pContext->merrno = 999; iError = 99; break;
	}
	return bRet;
}

bool iocpbase::oninit(cc *pContext, const int32_t& dwIoSize, const LPOVERLAPPED pOverlapBuff){
	if(notifyconnection(pContext)) return aread(pContext);
	return false;
}

// The read is not made directly to distribute CPU power fairly between the connections.
bool iocpbase::aread(cc* pcc){ // ��������ÿͻ����Ǹ���ϻ���Ŷ����������copy�ˡ��д��Ľ���
	bool ret = false; uint32_t nRetVal = 0, dwIoSize = 0, wsaError = 0, ulFlags = MSG_PARTIAL;
	LPWSABUF lpwsaBuf = NULL; readbuf* pread = NULL; BOOL bDeleted = FALSE;
	if (pcc != NULL){ // ����һ���첽��socket����
		if(NULL != (pread = new readbuf())){
			pread->prepare(pcc); lpwsaBuf = pread->getbuf();
			if (pcc->inc() > 0){
				nRetVal = WSARecv(pcc->msocket, lpwsaBuf, 1, (LPDWORD)&dwIoSize, (LPDWORD)&ulFlags, pread, NULL);
				if (nRetVal != SOCKET_ERROR || (wsaError = WSAGetLastError()) == WSA_IO_PENDING) return true;
				else pcc->merrno = wsaError;
			}
			release(pread);
		}
		pcc->dec();// IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
	}
	return false;
}

bool iocpbase::send(const char* p, const int32_t& len, cc* pcc){
	LPWSABUF pbuf = NULL; uint32_t nRetVal = 0, dwIoSize = 0, ulFlags = MSG_PARTIAL; int32_t wsaError = 0, iOnline = 0;
	if (pcc != NULL && pcc->msocket != INVALID_SOCKET){
		if (pcc->inc() > 0){	// ����һ��IO����֮ǰ��������IO��
			writebuf* wb = new writebuf(p, len);
			pbuf = wb->getbuf();
			if (len != pbuf->len) assert(0);
			//printf("%d - %d\r\n",len, pbuf->len);
			nRetVal = WSASend(pcc->msocket, pbuf, 1, (LPDWORD)&dwIoSize, ulFlags, wb, NULL);
			if (nRetVal != SOCKET_ERROR || (wsaError = WSAGetLastError()) == WSA_IO_PENDING){
				return true;
			}else{
				pcc->dec(); // IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
				release(wb);
			}
		}
	}
	return false;
}


bool iocpbase::onread(cc* pcc, const int32_t& len, const LPOVERLAPPED lpOverlap){
	bool ret = false; readbuf* pread = (readbuf*)lpOverlap;	int32_t i = 0, idx = 0; char path[1024] = { 0 };
	if (len == 0 || pread == NULL){ pcc->merrno = 999; return false; }// �п��ܿͻ��ιر��� socket
	if (pcc != NULL){
		// ����ÿ�� ClientContext ֻͶ��һ�� ��ȡ ���룬�����Ļ��� ClientContext ����������ľͲ���ͬ���ˡ�
		// ʵ���� һ��Ͷ�ݶ�� ��ȡ��Ҳû�б�Ҫ��������㷱�أ�������Ҳ���أ����ԼӴ��ȡ���ĳ��ȣ���û�б�Ҫ
		// Ͷ�ݶ����ȡ ����Ͷ�ݶ����ȡ �� ������ɰ���˳����ң�����Ҫ�԰���������������û�б�ҪͶ�ݶ����ȡ��
		// ���� Ͷ��ʱ�� LPWSABUF Ҳ֧��һ�������Ͷ�ݶ��������һ���� WSARecvͶ�ݶ��LPWSABUF��ȡ���壬���ҷ���ʱҲ������ģ�
		// ������ ʹ�� WSARecv ÿ��Ͷ��һ�� LPWSABUF �����ж��Ͷ�ݶ����󣩡�
		for (i = 0; i < len; i++){
			if (pcc->mbuf[i] == 0x0d){ pcc->cr = true; i++; if (i >= len) break; }
			if (pcc->cr && pcc->mbuf[i] == 0x0a){ // ̽��Ļس����С�
				if (pcc->crlf){ // ǰ���Ѿ��й��س������ˡ�˫�س����б�ʾ�����������ʼ��������
					pcc->haverequest = true; //printf("�����������ʼ�·��ļ���\r\n");
					ret = pcc->filetrans_push(this);
					if (!ret) return ret;
				} else { // ���س�
					pcc->crlf = true; if (i >= len){ assert(0); break; }
					if (!pcc->lastlineempty()){ // �����س�����ʱ����ǰ�в�Ϊ�գ���֤������Ч���ݣ�
						if (!pcc->get()){ // ��һ�б�����get
							if (!pcc->prepareget()) return false; // �����һ�в���get���ͷ��ش���
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
				if (pcc->haverequest) { printf("һ������ֻ������һ���ļ�������ж������󣬹ر����ӡ�\r\n"); return false; }// һ������ֻ������һ���ļ�������ж������󣬹ر����ӡ�
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
	if (dwIoSize == 0){ if (pcc->mtouchclose) { /*otprint("[�û�%d]�ر�����!\r\n", pcc->muserid);*/ pcc->setclose(); return false; }; }
	pcc->mtotal += dwIoSize; pcc->mtotalreset += dwIoSize;
	return true;
}



void iocpbase::printonlines(){
	cclock lock(cc::ccmutex); mapcc::iterator iter; std::string out, line; std::stringstream ss; int32_t i = 0; int64_t span;
	double speed;
	ss << "��������" << cc::cconlines.size();
	ss >> line; ss.clear(); out.append(line).append("\r\n");
	for (iter = cc::cconlines.begin(); iter != cc::cconlines.end(); iter++){
		ss << std::setfill('0') << std::setw(6) << i << "[" << iter->first << "][�û�" << std::setfill('0') << std::setw(10)<< iter->second->muserid << "][" << iter->second->getfile() << "][�";
		ss << iter->second->mlastAct - iter->second->mfirstAct << "][���" << iter->second->mioSize << "(" << std::fixed << std::setprecision(2) << (double(iter->second->mioSize * 100) / double(iter->second->getfilesize())) << "%%)][�ٶ�";
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