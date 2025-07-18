//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2021 LucKey Productions.
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

#include "../Core/Object.h"
#include "../Database/DbConnection.h"

namespace Dry
{

/// %Database cursor. Event handler could set P_FILTER to true to filter out a row from resultset and P_ABORT to true to stop further cursor events.
DRY_EVENT(E_DBCURSOR, DbCursor)
{
    DRY_PARAM(P_DBCONNECTION, DbConnection);    // DbConnection pointer
    DRY_PARAM(P_RESULTIMPL, ResultImpl);        // Underlying result object pointer (cannot be used in scripting)
    DRY_PARAM(P_SQL, SQL);                      // String
    DRY_PARAM(P_NUMCOLS, NumCols);              // unsigned
    DRY_PARAM(P_COLVALUES, ColValues);          // VariantVector
    DRY_PARAM(P_COLHEADERS, ColHeaders);        // StringVector
    DRY_PARAM(P_FILTER, Filter);                // bool [in]
    DRY_PARAM(P_ABORT, Abort);                  // bool [in]
}

}
