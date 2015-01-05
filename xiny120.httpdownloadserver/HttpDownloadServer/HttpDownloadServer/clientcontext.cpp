#include "IOCPBase.h"
#include "db.h"
#include "output.h"
#include <sstream>
#include "clientcontext.h"

extern const char* getbasepath();

NS_XINY120_BEGIN
//listcc cc::ccdels;
mapcc cc::cconlines;
std::atomic<uint64_t> cc::__mccid = 0;
std::recursive_mutex cc::ccmutex;
cc::cc(SOCKET socket) : msocket(socket), mios(1), mbuflen(2048), /*mbuflenw(1024 * 32),mbufw(0),*/ mclosetime(-1),
mftopen(false), mbuf(0),  mtouchclose(false), mtotal(0), mtotalreset(0), mspeedLimit(1024 * 100), mspeedlimit(false),
haverequest(false), cr(false), crlf(false), mfirstAct(time(NULL)), mlastAct(mfirstAct + 1), mlastTick(0), mioSize(0), mccid(__mccid++){
}
cc::~cc(){
	if (mspeedlimit) speedlimit::me()->ccgone(muserid);
	{cclock lock(ccmutex); cc::cconlines.erase(mccid); }
	//closesocket(msocket);
	//netbase->notifydisconnection(this);

	if (mbuf != 0) delete[] mbuf;
	//if (mbufw != 0) delete[] mbufw;
};

bool cc::init(){
	mbuf = new char[mbuflen]; //mbufw = new char[mbuflenw];
	cclock lock(ccmutex); cconlines[mccid] = this;
	return true;
}

int32_t cc::inc(){ 
	if (mios > 0) 
		mios++; 
	else otprint("inc fail.......................\r\n");
	return mios; 
};

int32_t cc::dec(){
	if (--mios <= 0){
		//netbase->closesocket(this, true);
		delete this;
		//{cclock lock(ccmutex); ccdels.push_back(this); }
		return 0;
	}
	return mios;
}
//void cc::touchclose(iocpbase* psvr){ mtouchclose = true; psvr->send("", 0, this);}

bool cc::close() {
	if (mclosetime <= 0){ if ((time(NULL) - mlastAct) > 60) { closesocket(false); return true; } }
	else if ((time(NULL) - mclosetime) > 10) { closesocket(false); return true; }
	return false;
};


int32_t cc::closesocket( bool bG){
	int32_t ret = 0, dwError = 0;
	if (bG && iocpbase::mDisConnectEx != NULL){
		if (inc() > 0){
			disconnectexbuf*  pOverlappedBuffer = new disconnectexbuf();
			BOOL bRet = iocpbase::mDisConnectEx(msocket, pOverlappedBuffer, 0, 0);
			if (bRet || ((dwError = WSAGetLastError()) == ERROR_IO_PENDING)){
				bRet = TRUE;
				//_tprintf(_T("m_pDisConnectEx��%d %d\r\n"),dwError,bRet);
				//_tprintf(_T("m_pDisConnectEx��%d PENDING��%d OK!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
				return true;
			}
			else{
				//_tprintf(_T("m_pDisConnectEx��%d PENDING��%d FAIL!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
			}
			//printf("\nCloseSocket\n");
			dec();
			iocpbase::release(pOverlappedBuffer);
		}
	}
	if (msocket != INVALID_SOCKET){ ret = ::closesocket(msocket); msocket = INVALID_SOCKET; }
	else{
		//_tprintf(_T("\r\n\r\nsocket�ر��ˡ�����û���ų����б� %s \t %d\r\n"),mp->m_ci.m_szUserName,mp->m_Socket);
	}
	//_tprintf(_T("\n��̫���ܵĴ���ɣ�socket�ر��ˡ�����û���ų����б� %s CCID��%I64u \t PENDING��%d\r\n"),mp->m_ci.m_tcUserName,mp->m_u64CCID,mp->m_nNumberOfPendlingIO);
	return ret;
}

void cc::requestclear(){
	std::map<std::string, std::string>::iterator iter;
	for (iter = request.begin(); iter != request.end(); iter++){ iter->second.clear(); }
	lastlineclear();
	haverequest = false;
}

bool cc::get(){
	std::map<std::string, std::string>::iterator iter = request.find(GET);
	if (iter != request.end()) return true;
	return false;
}

std::string cc::varget(const std::string& key){
	return varmap[key];
}

bool cc::prepareget(){
	int32_t i = 0, len = 0; std::stringstream ss(lastline); std::string txt, var, val;
	ss >> txt; std::transform(txt.begin(), txt.end(), txt.begin(), ::toupper);
	if (txt != GET) return false;
	request[GET] = GET;
	ss >> txt; txt = HttpUtility::URLDecode(txt); if (!txt.empty()) request[URI] = txt; len = txt.length();
	for (i = 0; i < len; i++){
		if (txt[i] == '='){ var = val; val.clear(); }
		else if (txt[i] == '&') { varmap[var] = val; var.clear(); val.clear(); }
		else if (txt[i] == '?') { request[GET] = val; val.clear(); var.clear(); }
		else val.push_back(txt[i]);
	}
	if ((!var.empty()) && (!val.empty())) varmap[var] = val;
	ss >> txt; if (!txt.empty()) request[VER] = txt;
	return true;
}

bool cc::preparereq(){
	int32_t i = 0, len = 0; std::string txt = lastline, var, val; len = txt.length();
	for (i = 0; i < len; i++){
		if (txt[i] == ':'){ var = val; val.clear(); }
		else val.push_back(txt[i]);
	}
	std::transform(var.begin(), var.end(), var.begin(), ::toupper);
	if ((!var.empty()) && (!val.empty()) && var == RANGE) request[var] = val;
	return true;
}

bool cc::filetrans_push(){
	mft.close(); std::string file; char buf[1024] = { 0 }; char filename[1024] = { 0 }; char ext[256] = { 0 }; const char* basepath = getbasepath();
	int32_t userid = 0, i = 0, len = 0, cl, s, e; bool ret = false; mftopen = false; std::string txt, var, val, resstr = "HTTP/1.1 200 OK"; std::stringstream ss;
	char cr[1024] = { 0 };
	std::string key = varget("key"); if (!key.empty()){
		file = db::me()->getfile(key, userid); if (!(file.empty() || userid == 0)){
			muserid = userid; _splitpath(file.c_str(), NULL, NULL, filename, ext);
			if (!mft.open(file,this)){
				otprint("[%d][%s]�ļ�������(%s)\r\n", muserid, key.c_str(), file.c_str()); return false;
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
					"Content-Disposition:attachment;filename=%s%s\r\n\r\n", resstr.c_str(), cl, cr, filename, ext);
				mspeedlimit = true;
				speedlimit::me()->cccame(muserid);
				iocpbase::me()->send(buf, strlen(buf), this);
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
			"Content-Length:10\r\n"
			"Server:httpdownloadserver\r\n"
			"Connection:close\r\n\r\nerror:403\r\n\r\n");
		iocpbase::me()->send(buf, strlen(buf), this);
		closesocket(true);
	}

	return false;
}

int32_t cc::filetrans_do(const int32_t& _count){
	bool status = true;
	if (!mftopen){ /*otprint("filetrans_do �����ļ�û�򿪣�\r\n");*/return true; }
	//int32_t count = _count; if (count > mbuflenw) count = mbuflenw;
	//count = mft.read(mbufw, count);
	//if (count > 0){ iocpbase::me()->send(mbufw, count, this); mioSize += count; mlastAct = time(NULL); }
	//else if (count == 0){ // ����ȫ���ļ���ɣ�����һ�����Źر�֪ͨ�����ӷ����б��������
	//	mftopen = false; mft.close();
	//	closesocket(true);
	//};
	//if ((status = mft.read(mbufw, count,this)) == false){
	//	mftopen = false; mft.close(); closesocket(true);
	//}
	//return count;
}

NS_XINY120_END