//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2023 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#ifdef DRY_IS_BUILDING
    #include "Dry.h"
#else
    #include <Dry/Dry.h>
#endif

// Macros with identifiers of c++ version.
#define DRY_CPP11_STANDARD (201103L)
#define DRY_CPP14_STANDARD (201402L)
#define DRY_CPP17_STANDARD (201703L)
#define DRY_CPP20_STANDARD (202000L) // speculative

// Macro that can be used to identify c++ version.
#ifndef DRY_CPLUSPLUS
    #if defined(_MSVC_LANG) && !defined(__clang__)
        // Note: VC14.0/1900 (VS2015) lacks too much from C++14
        #if (_MSC_VER == 1900)
            #define DRY_CPLUSPLUS DRY_CPP11_STANDARD
        #else
            #define DRY_CPLUSPLUS _MSVC_LANG
        #endif
    #else
        #define DRY_CPLUSPLUS __cplusplus
    #endif
#endif

// Macros with a strong guarantee of what c++ version is available.
#define DRY_CPP11_OR_LATER (DRY_CPLUSPLUS >= DRY_CPP11_STANDARD)
#define DRY_CPP14_OR_LATER (DRY_CPLUSPLUS >= DRY_CPP14_STANDARD)
#define DRY_CPP17_OR_LATER (DRY_CPLUSPLUS >= DRY_CPP17_STANDARD)

// Macros for specific c++ versions
#if DRY_CPP11_OR_LATER
    // Least match
    #define DRY_CPP11(x) x
    #define DRY_CPP14(x)
    #define DRY_CPP17(x)
    // Exact match
    #define DRY_CPP11_ONLY(x) x
    #define DRY_CPP14_ONLY(x)
    #define DRY_CPP17_ONLY(x)
#elif DRY_CPP14_OR_LATER
    // Least match
    #define DRY_CPP11(x) x
    #define DRY_CPP14(x) x
    #define DRY_CPP17(x)
    // Exact match
    #define DRY_CPP11_ONLY(x)
    #define DRY_CPP14_ONLY(x) x
    #define DRY_CPP17_ONLY(x)
#elif DRY_CPP17_OR_LATER
    // Least match
    #define DRY_CPP11(x) x
    #define DRY_CPP14(x) x
    #define DRY_CPP17(x) x
    // Exact match
    #define DRY_CPP11_ONLY(x)
    #define DRY_CPP14_ONLY(x)
    #define DRY_CPP17_ONLY(x) x
#else
    #error Compiler does not support required c++ standard. How did you get here?
#endif

// Case insensitive string comparison.
#ifdef _WIN32
   #define DRY_STRICMP(a,b) stricmp(a,b)
   #define DRY_STRNICMP(a,b,n) strnicmp(a,b,n)
#else
   #define DRY_STRICMP(a,b) strcasecmp(a,b)
   #define DRY_STRNICMP(a,b,n) strncasecmp(a,b,n)
#endif
