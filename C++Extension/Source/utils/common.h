#pragma once

#include <new>  // Must include this to use "placement new"

#include <assert.h>
#include <time.h>

#include <wchar.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <stdarg.h>
#include <limits.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <thread>
#include <mutex>

#include <cmath>
#include <exception>
#include <stdexcept>

#include <chrono>

#ifdef _MSC_VER
#include <intrin.h>
#include <emmintrin.h>
#ifndef USE_INTRINSICS
#define USE_INTRINSICS
#endif
#else
#if (!defined(__arm64__))
#include <pmmintrin.h>
#include <emmintrin.h>
#include <mmintrin.h>
#ifndef USE_INTRINSICS
#define USE_INTRINSICS
#endif
#endif
#endif

#define M_GEOM_USE_DOUBLE

#ifdef M_GEOM_USE_DOUBLE
typedef double treal;
#else
typedef float treal;
#endif

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif
