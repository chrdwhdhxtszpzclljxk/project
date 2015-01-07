#include "output.h"
//#include "buffer.h"
#include "IOCPBase.h"
#include "clientcontext.h"
#include "fileinfo.h"

#pragma warning(disable:4996)
NS_XINY120_BEGIN

/*
bool filetrans::open(const std::string& fn){
	if (fp != 0) fclose(fp); path = fn;
	if ((fp = fopen(fn.c_str(), "rb")) == 0){
		otprint("%d", errno);
		return false;
	}
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
		if (end != -1){ // range 指定了结束标记。
			if (readed >= end) return 0; // 已经读取的字节大于等于range指定的字节，停止读取。
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

int32_t filetrans::getsize(){ return filesize; }
*/

filetrans::filetrans(cc* pcc) : f(INVALID_HANDLE_VALUE), filesize(0), end(-1), mpcc(pcc), mbuftotal(1024 * 15),mbytestotal(0),
mcurtick(GetTickCount()){
	mbuf = new char[mbuftotal];
	mpendding.clear();
	Offset = OffsetHigh = 0;
};

bool filetrans::open(const std::string& fn){
	if (f != INVALID_HANDLE_VALUE) CloseHandle(f); path = fn;
	if ((f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE){//if ((fp = fopen(fn.c_str(), "rb")) == 0){
		otprint("%d", errno);
		return false;
	}
	LARGE_INTEGER li; filesize = 0;
	if (GetFileSizeEx(f, &li)) filesize = li.QuadPart;
	end = -1; Offset = OffsetHigh = 0;
	return true;
}

void filetrans::close(){ 
	if (f != INVALID_HANDLE_VALUE){
		//CancelIoEx(f, NULL);
		CloseHandle(f); f = INVALID_HANDLE_VALUE;

	}
}
int32_t filetrans::setend(const int32_t& _end){ end = _end; return end; }
int32_t filetrans::seek(const int32_t& pos){
	if (f != INVALID_HANDLE_VALUE){ 
		//len = ::SetFilePointer(f, pos, NULL, FILE_BEGIN); readed += len; return len;
		Offset += pos; return pos;
	}
	return -1; 
};

int32_t filetrans::speedpush(const speedshot& ss){
	//cclock _l(mmutxpendding);
	msq.push(ss); mbytestotal += ss.mbytes; mcurtick = GetTickCount();
	while (mcurtick - (msq.front().mtick) > 1000){
		mbytestotal -= msq.front().mbytes;
		msq.pop();
	}
	return mbytestotal;
}

int32_t filetrans::speed(){
	//cclock _l(mmutxpendding);
	double span = 1;
	if (msq.size() > 0){
		span = (GetTickCount() - msq.front().mtick) / 1000.0f;
		if (span < 0) span = 1;
	}
	return double(mbytestotal) / span;
}

VOID CALLBACK filetrans::filereadRoutine(DWORD code, DWORD readed, LPOVERLAPPED ol){//定义一个简单的回调函数
	filetrans* pThis = (filetrans*)ol;
	if (code == 0 && readed > 0){
		iocpbase::me()->send(pThis->mbuf, readed, pThis->mpcc);
	}
	pThis->Offset += readed;
	speedshot ss; ss.mbytes = readed; ss.mtick = GetTickCount();
	pThis->speedpush(ss);
	if (pThis->mpcc->dec() > 0)
		pThis->mpendding.clear();
	else
		;// printf("小于0！小于0！小于0！小于0！小于0！\r\n");
}

int32_t filetrans::read(const int32_t& _len){
	int32_t len = _len, speednow = 0,speedlimit; BOOL status = FALSE; DWORD er;
	if (len > mbuftotal) len = mbuftotal;
	len = mbuftotal; speednow = speed(); speedlimit = speedlimit::me()->getspeed(mpcc->muserid);
	if (speednow >= speedlimit){
		return 0;
	}
	if (speednow > 0 && speednow <= speedlimit){
		speedlimit::me()->speedmore += (speedlimit - speednow);
	}
	if (mpendding.test_and_set()) return 0;
	if (f != INVALID_HANDLE_VALUE){
		if (end != -1){ // range 指定了结束标记。
			if (Offset >= end){ close(); return -1; } // 已经读取的字节大于等于range指定的字节，停止读取。
			if ((Offset + len) > end){ len = end - Offset; }
		}
		DWORD lened = 0;
		memset(mbuf, 0, len);
		if (mpcc->inc() > 0){
			if ((status = ReadFileEx(f, mbuf, len, this, filereadRoutine)) == TRUE){ // 读取文件成功。直接发送。
				return len;
			}
			else { if ((er = GetLastError()) == ERROR_HANDLE_EOF){ } }// 异步处理中...
			if (mpcc->dec() == 0){ assert(0); return -1; }
		}
	}
	mpendding.clear();
	return -1;
}

void filetrans::send(){

}

int32_t filetrans::getsize(){ return filesize; }


NS_XINY120_END