#ifndef _HDS_NS_H
#define _HDS_NS_H
#include <stdio.h>
#include <stdint.h>
#include <mutex>

#define NS_XINY120_BEGIN namespace xiny120{
#define NS_XINY120_END };

typedef std::lock_guard<std::recursive_mutex> cclock;

#define DECLARE_SINGLETON_FUNC(c) static c* me(){ \
	static c* obj = NULL; \
	if(obj == NULL){ \
		obj = new c;\
		if(!obj->init()){\
			delete obj; obj = NULL;\
		}\
	}\
	return obj;\
}


#endif