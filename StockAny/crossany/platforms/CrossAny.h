#ifndef _CROSSANY_H
#define _CROSSANY_H

#define NS_CROSSANY_BEGIN namespace crossany {
#define NS_CROSSANY_END }
#define NS_CROSSANY using namespace crossany;

#define CA_ASSERT(...)

#define NULLPTR NULL;
#define CADEL(p) {delete p; p = NULLPTR;}

#include <stdint.h>


#endif
