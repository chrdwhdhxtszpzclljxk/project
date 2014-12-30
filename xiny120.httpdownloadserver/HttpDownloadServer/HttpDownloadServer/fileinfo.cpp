#include "fileinfo.h"
#include "output.h"
#pragma warning(disable:4996)
NS_XINY120_BEGIN

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

NS_XINY120_END