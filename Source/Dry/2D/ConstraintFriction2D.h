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

#include "../2D/Constraint2D.h"

namespace Dry
{

/// 2D friction constraint component.
class DRY_API ConstraintFriction2D : public Constraint2D
{
    DRY_OBJECT(ConstraintFriction2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintFriction2D(Context* context);
    /// Destruct.
    ~ConstraintFriction2D() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set anchor.
    void SetAnchor(const Vector2& anchor);
    /// Set max force.
    void SetMaxForce(float maxForce);
    /// Set max torque.
    void SetMaxTorque(float maxTorque);

    /// Return anchor.
    const Vector2& GetAnchor() const { return anchor_; }

    /// Set max force.
    float GetMaxForce() const { return jointDef_.maxForce; }

    /// Set max torque.
    float GetMaxTorque() const { return jointDef_.maxTorque; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2FrictionJointDef jointDef_;
    /// Anchor.
    Vector2 anchor_;
};

}
