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
#include <Dry/Math/Polynomial.h>

using namespace Dry;

namespace Dry
{
class Controls;
}

class MovingPlatform : public LogicComponent
{
    DRY_OBJECT(MovingPlatform, LogicComponent);

    void FixedUpdate(float timeStep) override;

public:

    MovingPlatform(Context* context);
    virtual ~MovingPlatform();

    static void RegisterObject(Context* context);


    void Initialize(Node* liftNode, const Vector3& finishPosition, bool updateBodyOnPlatform=true);
    void SetPlatformSpeed(float speed) { speed_ = speed; }

protected:
    WeakPtr<Node> platformNode_;
    WeakPtr<Node> platformVolumdNode_;
    SharedPtr<ValueAnimation> movement_;

    Vector3 initialPosition_;
    Vector3 finishPosition_;

    Polynomial sway_;

    float speed_;
};




