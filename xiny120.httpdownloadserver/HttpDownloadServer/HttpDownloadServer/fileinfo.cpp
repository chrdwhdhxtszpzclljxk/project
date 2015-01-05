#include "output.h"
#include "buffer.h"
#include "IOCPBase.h"
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

bool filetrans::open(const std::string& fn,cc* pcc){
	if (f != INVALID_HANDLE_VALUE) CloseHandle(f); path = fn;
	if ((f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE){//if ((fp = fopen(fn.c_str(), "rb")) == 0){
		otprint("%d", errno);
		return false;
	}
	LARGE_INTEGER li; filesize = 0;
	if (GetFileSizeEx(f, &li)) filesize = li.QuadPart;
	end = -1; readed = 0;
	iocpbase::me()->file2iocp(f, pcc);
	return true;
}

void filetrans::close(){ if (f != INVALID_HANDLE_VALUE) CloseHandle(f); f = INVALID_HANDLE_VALUE; }
int32_t filetrans::setend(const int32_t& _end){ end = _end; return end; }
int32_t filetrans::seek(const int32_t& pos){
	int32_t len;
	if (f != INVALID_HANDLE_VALUE){ 
		len = ::SetFilePointer(f, pos, NULL, FILE_BEGIN); readed += len; return len;
	}
	return -1; 
};
int32_t filetrans::read(char* _buf, const int32_t& _len,cc* pcc){
	int32_t len = _len; BOOL status = FALSE; DWORD er;
	if (f != INVALID_HANDLE_VALUE){
		if (end != -1){ // range 指定了结束标记。
			if (readed >= end){ close(); return 0; } // 已经读取的字节大于等于range指定的字节，停止读取。
			if ((readed + len) > end){ len = end - readed; }
		}
		//len = fread(buf, 1, len, fp);
		DWORD lened = 0; readfilebuf* buf = new readfilebuf();
		if ((status = ReadFile(f, _buf, len, NULL, buf)) == TRUE ){ // 读取文件成功。直接发送。
			send(pcc);
			return true;
		} else if ((er = GetLastError()) == ERROR_IO_PENDING) return true; // 异步处理中...
	}
	return false;
}

void filetrans::send(cc* pcc){

}

int32_t filetrans::getsize(){ return filesize; }


NS_XINY120_END