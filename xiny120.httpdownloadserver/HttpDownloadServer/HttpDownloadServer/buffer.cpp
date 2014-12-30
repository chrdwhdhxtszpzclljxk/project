#include "buffer.h"
#include <assert.h>
#include "clientcontext.h"


NS_XINY120_BEGIN


bool readbuf::prepare(cc* pcc){ buf[0].buf = pcc->mbuf; buf[0].len = pcc->mbuflen; return true; }

writebuf::writebuf(const char* p, const int32_t& len) : basebuf(write){
	try{ buf[0].len = len; buf[0].buf = new char[len]; memcpy(buf[0].buf, p, len); }
	catch (...){ assert(0); }
}

writebuf::~writebuf(){ delete[] buf[0].buf; }

NS_XINY120_END