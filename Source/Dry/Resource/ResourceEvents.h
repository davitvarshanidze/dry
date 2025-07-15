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

#include "../Core/Object.h"

namespace Dry
{

/// Resource reloading started.
DRY_EVENT(E_RELOADSTARTED, ReloadStarted)
{
}

/// Resource reloading finished successfully.
DRY_EVENT(E_RELOADFINISHED, ReloadFinished)
{
}

/// Resource reloading failed.
DRY_EVENT(E_RELOADFAILED, ReloadFailed)
{
}

/// Tracked file changed in the resource directories.
DRY_EVENT(E_FILECHANGED, FileChanged)
{
    DRY_PARAM(P_FILENAME, FileName);                    // String
    DRY_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Resource loading failed.
DRY_EVENT(E_LOADFAILED, LoadFailed)
{
    DRY_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Resource not found.
DRY_EVENT(E_RESOURCENOTFOUND, ResourceNotFound)
{
    DRY_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Unknown resource type.
DRY_EVENT(E_UNKNOWNRESOURCETYPE, UnknownResourceType)
{
    DRY_PARAM(P_RESOURCETYPE, ResourceType);            // StringHash
}

/// Resource background loading finished.
DRY_EVENT(E_RESOURCEBACKGROUNDLOADED, ResourceBackgroundLoaded)
{
    DRY_PARAM(P_RESOURCENAME, ResourceName);            // String
    DRY_PARAM(P_SUCCESS, Success);                      // bool
    DRY_PARAM(P_RESOURCE, Resource);                    // Resource pointer
}

/// Language changed.
DRY_EVENT(E_CHANGELANGUAGE, ChangeLanguage)
{
}

}
