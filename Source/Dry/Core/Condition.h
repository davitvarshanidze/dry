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

namespace Dry
{

/// %Condition on which a thread can wait.
class DRY_API Condition
{
public:
    /// Construct.
    Condition();

    /// Destruct.
    ~Condition();

    /// Set the condition. Will be automatically reset once a waiting thread wakes up.
    void Set();

    /// Wait on the condition.
    void Wait();

private:
#ifndef _WIN32
    /// Mutex for the event, necessary for pthreads-based implementation.
    void* mutex_;
#endif
    /// Operating system specific event.
    void* event_;
};

}
