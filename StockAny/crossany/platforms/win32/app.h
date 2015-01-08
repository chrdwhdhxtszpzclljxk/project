#ifndef _CAAPP_H
#define _CAAPP_H
#include "../CrossAny.h"
#include "../appbase.h"
#include <Windows.h>

NS_CROSSANY_BEGIN

class App : public appbase{
public:
	static App* me();
	virtual bool init(){ return true; };
	virtual int run();
	HINSTANCE hInst;
protected:
	App();
	~App();
protected:

	virtual bool oninit() { return true; };
	virtual void onexit(){}
	virtual void lostfocus() {};
	virtual void getfocus() {};
	virtual void initGLContextAttrs() {};
	virtual Platform getCurrentPlatform() { return Platform::OS_WINDOWS; };

};

NS_CROSSANY_END
#endif