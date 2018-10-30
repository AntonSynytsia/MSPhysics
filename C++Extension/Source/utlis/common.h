#pragma once

#include <new> // Must include this to use "placement new"

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

#include <assert.h>
#include <time.h>

#include <wchar.h>
#include <stdio.h>