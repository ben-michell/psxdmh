// psxdmh/src/global.h
// Global definitions for psxdmh.
// Copyright (c) 2016,2021 Ben Michell <https://www.muryan.com/>. All rights reserved.

// This file is part of psxdmh.
//
// psxdmh is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// psxdmh is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// psxdmh.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PSXDMH_SRC_GLOBAL_H
#define PSXDMH_SRC_GLOBAL_H


// Determine the target platform.
#if defined(__APPLE__)
    #define PSXDMH_TARGET_MACOS
#elif defined(_WIN32)
    #define PSXDMH_TARGET_WINDOWS
#else // Target.
    #error Unknown target platform.
#endif // Target.


// Disable MSVC foolishness.
#ifdef _MSC_VER
    #pragma warning(disable : 4786)
    #pragma warning(disable : 4996)
    #pragma warning(disable : 26495)
    #define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER


// Common includes.
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <csetjmp>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>


// Target-specific includes.
#if defined(PSXDMH_TARGET_MACOS)
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
#elif defined(PSXDMH_TARGET_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <float.h>
    #include <io.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #ifndef isnan
        #define isnan(f)    _isnan(f)
    #endif // isnan
    #define strcasecmp      stricmp
    #define strncasecmp     strnicmp
#endif // Target.


// Ensure the min and max macros aren't defined.
#undef min
#undef max


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Maths constants.
#ifndef M_PI
    #define M_PI            (3.1415926535897932384626433832795)
#endif // M_PI
#ifndef M_SQRT2
    #define M_SQRT2         (1.4142135623730950488016887242097)
#endif // M_SQRT2


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Convert the value of a preprocessor symbol to a string.
#define PSXDMH_STRINGIFY(X)         PSXDMH_STRINGIFY_2(X)
#define PSXDMH_STRINGIFY_2(X)       #X


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Number of items in an array.
#ifndef numberof
    #define numberof(A)     (sizeof(A) / sizeof((A)[0]))
#endif // numberof


#endif // PSXDMH_SRC_GLOBAL_H
