#ifndef __CROSSANY_APPBASE_H__
#define __CROSSANY_APPBASE_H__
#include "CrossAny.h"

NS_CROSSANY_BEGIN

class appbase{
public:
	enum class Platform{
		OS_WINDOWS, OS_LINUX, OS_MAC,
		OS_ANDROID, OS_IPHONE, OS_IPAD, OS_BLACKBERRY,
		OS_NACL, OS_EMSCRIPTEN, OS_TIZEN,
		OS_WINRT,OS_WP8
	};
public:
	//appbase();
	virtual ~appbase() {};

	virtual bool dolaunching() = 0;
	virtual void lostfocus() = 0;
	virtual void getfocus() = 0;
	virtual void initGLContextAttrs() {};
	virtual Platform getCurrentPlatform() = 0;
};

NS_CROSSANY_END
#endif

