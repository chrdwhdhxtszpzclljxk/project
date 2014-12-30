#include <stdarg.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <process.h>
#include <time.h>
#include "output.h"
#pragma warning(disable:4996)

static bool bout = true;

int32_t otprint(const char* format, ...){
	if (!bout) return 0;
	va_list args; int32_t len;const int32_t clen = 2048; char buffer[clen] = { 0 }; time_t now = time(NULL); tm tm1; localtime_s(&tm1, &now);
	printf("%02d-%02d %02d:%02d:%02d ", tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
	va_start(args, format);	// retrieve the variable arguments
	len = _vscprintf(format, args) + 1; // _vscprintf doesn't count// terminating '\0'
	if (len < clen) vsprintf_s(buffer,len, format, args); // C4996
	else sprintf(buffer,"output overflow!");
	puts(buffer);
	//delete [] buffer;
	return 0;
}

