#ifndef __PLATFORM_CAAPP_H_
#define __PLATFORM_CAAPP_H_

#include "CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
#include "platform/mac/CCApplication-mac.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "platform/ios/CCApplication-ios.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/CCApplication-android.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#include "win32/app.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8
#include "platform/winrt/CCApplication.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_LINUX
#include "platform/linux/CCApplication-linux.h"
#endif

#endif