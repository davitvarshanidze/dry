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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../2D/ConstraintDistance2D.h"
#include "../2D/PhysicsUtils2D.h"
#include "../2D/RigidBody2D.h"

#include "../DebugNew.h"

namespace Dry
{

extern const char* DRY_2D_CATEGORY;

ConstraintDistance2D::ConstraintDistance2D(Context* context) :
    Constraint2D(context),
    ownerBodyAnchor_(Vector2::ZERO),
    otherBodyAnchor_(Vector2::ZERO)
{

}

ConstraintDistance2D::~ConstraintDistance2D() = default;

void ConstraintDistance2D::RegisterObject(Context* context)
{
    context->RegisterFactory<ConstraintDistance2D>(DRY_2D_CATEGORY);

    DRY_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Owner Body Anchor", GetOwnerBodyAnchor, SetOwnerBodyAnchor, Vector2, Vector2::ZERO, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Other Body Anchor", GetOtherBodyAnchor, SetOtherBodyAnchor, Vector2, Vector2::ZERO, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Frequency Hz", GetFrequencyHz, SetFrequencyHz, float, 0.0f, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Damping Ratio", GetDampingRatio, SetDampingRatio, float, 0.0f, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, 1.0f, AM_DEFAULT);
    DRY_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintDistance2D::SetOwnerBodyAnchor(const Vector2& anchor)
{
    if (anchor == ownerBodyAnchor_)
        return;

    ownerBodyAnchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetOtherBodyAnchor(const Vector2& anchor)
{
    if (anchor == otherBodyAnchor_)
        return;

    otherBodyAnchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetFrequencyHz(float frequencyHz)
{
    if (frequencyHz == jointDef_.frequencyHz)
        return;

    jointDef_.frequencyHz = frequencyHz;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetFrequency(frequencyHz);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetDampingRatio(float dampingRatio)
{
    if (dampingRatio == jointDef_.dampingRatio)
        return;

    jointDef_.dampingRatio = dampingRatio;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetDampingRatio(dampingRatio);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetLength(float length)
{
    if (length == jointDef_.length)
        return;

    jointDef_.length = length;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetLength(length);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

b2JointDef* ConstraintDistance2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB, ToB2Vec2(ownerBodyAnchor_), ToB2Vec2(otherBodyAnchor_));

    return &jointDef_;
}

}
