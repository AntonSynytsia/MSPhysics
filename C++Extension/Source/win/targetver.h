#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

//#include <SDKDDKVer.h>
#include <WinSDKVer.h>

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#ifdef WINVER
#undef WINVER
#endif

// 64 bit
#if defined(_M_X64) || defined(__amd64__)

#if defined(RUBY_VERSION20) || defined(RUBY_VERSION22) || defined(RUBY_VERSION25) || defined(RUBY_VERSION27)
#define WINVER _WIN32_WINNT_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#else // 32 bit

#if defined(RUBY_VERSION18)
#define WINVER _WIN32_WINNT_WINXP
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#elif defined(RUBY_VERSION20)
#define WINVER _WIN32_WINNT_WINXP
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif

#endif
