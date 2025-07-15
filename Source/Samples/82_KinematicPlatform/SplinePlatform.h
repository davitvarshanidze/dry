//
// Copyright (c) 2008-2019 the Urho3D project.
// Copyright (c) 2022-2022 LucKey Productions.
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

#include <Dry/Scene/LogicComponent.h>

using namespace Dry;

namespace Dry
{

class SplinePath;

}

class SplinePlatform : public LogicComponent
{
    DRY_OBJECT(SplinePlatform, LogicComponent);
public:

    SplinePlatform(Context* context);
    virtual ~SplinePlatform();

    static void RegisterObject(Context* context);

    virtual void FixedUpdate(float timeStep);

    void Initialize(Node *node);

protected:
    WeakPtr<Node> splinePathNode_;
    WeakPtr<SplinePath> splinePath_;
    WeakPtr<Node> controlNode_;
    WeakPtr<Node> testNode_;
    Vector3 prevPos_;
    Vector3 curPos_;
    Matrix3x4 prevMat_;
    Matrix3x4 curMat_;
    float rotation_;
};




