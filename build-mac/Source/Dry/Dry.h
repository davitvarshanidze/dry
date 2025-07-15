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

#ifndef DRY_API_H
#define DRY_API_H

/* #undef DRY_STATIC_DEFINE */
#define DRY_OPENGL
/* #undef DRY_D3D11 */
/* #undef DRY_SSE */
/* #undef DRY_DATABASE_ODBC */
/* #undef DRY_DATABASE_SQLITE */
/* #undef DRY_LUAJIT */
/* #undef DRY_TESTING */

/* #undef CLANG_PRE_STANDARD */

#ifdef DRY_STATIC_DEFINE
#  define DRY_API
#  define DRY_NO_EXPORT
#else
#  ifndef DRY_API
#    ifdef Dry_EXPORTS
        /* We are building this library */
#      define DRY_API __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define DRY_API __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef DRY_NO_EXPORT
#    define DRY_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef DRY_DEPRECATED
#  define DRY_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef DRY_DEPRECATED_EXPORT
#  define DRY_DEPRECATED_EXPORT DRY_API DRY_DEPRECATED
#endif

#ifndef DRY_DEPRECATED_NO_EXPORT
#  define DRY_DEPRECATED_NO_EXPORT DRY_NO_EXPORT DRY_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DRY_NO_DEPRECATED
#    define DRY_NO_DEPRECATED
#  endif
#endif

#define NONSCRIPTABLE 

#endif
