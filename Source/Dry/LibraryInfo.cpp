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

#include "Precompiled.h"

#include "LibraryInfo.h"
#include "librevision.h"

namespace Dry
{

const char* GetRevision()
{
    return revision;
}

const char* GetCompilerDefines()
{
    return ""
    "#define DRY_OPENGL\n"
#ifdef DRY_SSE
    "#define DRY_SSE\n"
#endif
#ifdef DRY_DATABASE_ODBC
    "#define DRY_DATABASE_ODBC\n"
#elif defined(DRY_DATABASE_SQLITE)
    "#define DRY_DATABASE_SQLITE\n"
#endif
#ifdef DRY_TESTING
    "#define DRY_TESTING\n"
#endif
    ;
}

}
