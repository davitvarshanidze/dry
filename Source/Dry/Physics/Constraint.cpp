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
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Physics/Constraint.h"
#include "../Physics/PhysicsUtils.h"
#include "../Physics/PhysicsWorld.h"
#include "../Physics/RigidBody.h"
#include "../Scene/Scene.h"

#include <Bullet/BulletDynamics/ConstraintSolver/btConeTwistConstraint.h>
#include <Bullet/BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <Bullet/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h>
#include <Bullet/BulletDynamics/ConstraintSolver/btSliderConstraint.h>
#include <Bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Dry
{

static const char* typeNames[] =
{
    "Point",
    "Hinge",
    "Slider",
    "ConeTwist",
    nullptr
};

extern const char* DRY_PHYSICS_CATEGORY;

Constraint::Constraint(Context* context): Component(context),
    constraintType_{ CONSTRAINT_POINT },
    position_{ Vector3::ZERO },
    rotation_{ Quaternion::IDENTITY },
    otherPosition_{ Vector3::ZERO },
    otherRotation_{ Quaternion::IDENTITY },
    highLimit_{ Vector2::ZERO },
    lowLimit_{ Vector2::ZERO },
    erp_{ 0.0f },
    cfm_{ 0.0f },
    softness_{ 0.9f },
    biasFactor_{ 0.3f },
    relaxationFactor_{ 1.0f },
    otherBodyNodeID_{ 0 },
    disableCollision_{ false },
    recreateConstraint_{ true },
    framesDirty_{ false },
    retryCreation_{ false }
{
}

Constraint::~Constraint()
{
    ReleaseConstraint();

    if (physicsWorld_)
        physicsWorld_->RemoveConstraint(this);
}

void Constraint::RegisterObject(Context* context)
{
    context->RegisterFactory<Constraint>(DRY_PHYSICS_CATEGORY);

    DRY_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    DRY_ENUM_ATTRIBUTE_EX("Constraint Type", constraintType_, MarkConstraintDirty, typeNames, CONSTRAINT_POINT, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Position", Vector3, position_, AdjustOtherBodyPosition, Vector3::ZERO, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Rotation", Quaternion, rotation_, MarkFramesDirty, Quaternion::IDENTITY, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Other Body Position", Vector3, otherPosition_, MarkFramesDirty, Vector3::ZERO, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Other Body Rotation", Quaternion, otherRotation_, MarkFramesDirty, Quaternion::IDENTITY, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Other Body NodeID", unsigned, otherBodyNodeID_, MarkConstraintDirty, 0, AM_DEFAULT | AM_NODEID);
    DRY_ACCESSOR_ATTRIBUTE("High Limit", GetHighLimit, SetHighLimit, Vector2, Vector2::ZERO, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Low Limit", GetLowLimit, SetLowLimit, Vector2, Vector2::ZERO, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("ERP Parameter", GetERP, SetERP, float, 0.0f, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("CFM Parameter", GetCFM, SetCFM, float, 0.0f, AM_DEFAULT);
    DRY_ATTRIBUTE_EX("Disable Collision", bool, disableCollision_, MarkConstraintDirty, false, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Softness", GetSoftness, SetSoftness, float, 0.9f, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Bias Factor", GetBiasFactor, SetBiasFactor, float, 0.3f, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Relaxation Factor", GetRelaxationFactor, SetRelaxationFactor, float, 1.0f, AM_DEFAULT);
}

void Constraint::ApplyAttributes()
{
    if (recreateConstraint_)
    {
        if (otherBody_)
            otherBody_->RemoveConstraint(this);

        otherBody_.Reset();

        Scene* scene = GetScene();
        if (scene && otherBodyNodeID_)
        {
            Node* otherNode = scene->GetNode(otherBodyNodeID_);
            if (otherNode)
                otherBody_ = otherNode->GetComponent<RigidBody>();
        }

        CreateConstraint();
    }
    else if (framesDirty_)
    {
        ApplyFrames();
        framesDirty_ = false;
    }
}

void Constraint::OnSetEnabled()
{
    if (constraint_)
        constraint_->setEnabled(IsEnabledEffective());
}

void Constraint::GetDependencyNodes(PODVector<Node*>& dest)
{
    if (otherBody_ && otherBody_->GetNode())
        dest.Push(otherBody_->GetNode());
}

void Constraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && physicsWorld_ && constraint_)
    {
        physicsWorld_->SetDebugRenderer(debug);
        physicsWorld_->SetDebugDepthTest(depthTest);
        physicsWorld_->GetWorld()->debugDrawConstraint(constraint_.Get());
        physicsWorld_->SetDebugRenderer(nullptr);
    }
}

void Constraint::SetConstraintType(ConstraintType type)
{
    if (type != constraintType_ || !constraint_)
    {
        constraintType_ = type;
        CreateConstraint();
        MarkNetworkUpdate();
    }
}

void Constraint::SetOtherBody(RigidBody* body)
{
    if (otherBody_ != body)
    {
        if (otherBody_)
            otherBody_->RemoveConstraint(this);

        otherBody_ = body;

        // Update the connected body attribute
        Node* otherNode = otherBody_ ? otherBody_->GetNode() : nullptr;
        otherBodyNodeID_ = otherNode ? otherNode->GetID() : 0;

        CreateConstraint();
        MarkNetworkUpdate();
    }
}

void Constraint::SetPosition(const Vector3& position)
{
    if (position != position_)
    {
        position_ = position;
        ApplyFrames();
        MarkNetworkUpdate();
    }
}

void Constraint::SetRotation(const Quaternion& rotation)
{
    if (rotation != rotation_)
    {
        rotation_ = rotation;
        ApplyFrames();
        MarkNetworkUpdate();
    }
}

void Constraint::SetAxis(const Vector3& axis)
{
    switch (constraintType_)
    {
    case CONSTRAINT_POINT:
    case CONSTRAINT_HINGE:
        rotation_ = Quaternion(Vector3::FORWARD, axis);
        break;

    case CONSTRAINT_SLIDER:
    case CONSTRAINT_CONETWIST:
        rotation_ = Quaternion(Vector3::RIGHT, axis);
        break;

    default:
        break;
    }

    ApplyFrames();
    MarkNetworkUpdate();
}

void Constraint::SetOtherPosition(const Vector3& position)
{
    if (position != otherPosition_)
    {
        otherPosition_ = position;
        ApplyFrames();
        MarkNetworkUpdate();
    }
}

void Constraint::SetOtherRotation(const Quaternion& rotation)
{
    if (rotation != otherRotation_)
    {
        otherRotation_ = rotation;
        ApplyFrames();
        MarkNetworkUpdate();
    }
}

void Constraint::SetOtherAxis(const Vector3& axis)
{
    switch (constraintType_)
    {
    case CONSTRAINT_POINT:
    case CONSTRAINT_HINGE:
        otherRotation_ = Quaternion(Vector3::FORWARD, axis);
        break;

    case CONSTRAINT_SLIDER:
    case CONSTRAINT_CONETWIST:
        otherRotation_ = Quaternion(Vector3::RIGHT, axis);
        break;

    default:
        break;
    }

    ApplyFrames();
    MarkNetworkUpdate();
}

void Constraint::SetWorldPosition(const Vector3& position)
{
    if (constraint_)
    {
        btTransform ownBodyInverse = constraint_->getRigidBodyA().getWorldTransform().inverse();
        btTransform otherBodyInverse = constraint_->getRigidBodyB().getWorldTransform().inverse();
        btVector3 worldPos = ToBtVector3(position);
        position_ = (ToVector3(ownBodyInverse * worldPos) + ownBody_->GetCenterOfMass()) / cachedWorldScale_;
        otherPosition_ = ToVector3(otherBodyInverse * worldPos);
        if (otherBody_)
        {
            otherPosition_ += otherBody_->GetCenterOfMass();
            otherPosition_ /= otherBody_->GetNode()->GetWorldScale();
        }
        ApplyFrames();
        MarkNetworkUpdate();
    }
    else
        DRY_LOGWARNING("Constraint not created, world position could not be stored");
}

void Constraint::SetHighLimit(const Vector2& limit)
{
    if (limit != highLimit_)
    {
        highLimit_ = limit;
        ApplyLimits();
        MarkNetworkUpdate();
    }
}

void Constraint::SetLowLimit(const Vector2& limit)
{
    if (limit != lowLimit_)
    {
        lowLimit_ = limit;
        ApplyLimits();
        MarkNetworkUpdate();
    }
}

void Constraint::SetERP(float erp)
{
    erp = Max(erp, 0.0f);

    if (erp != erp_)
    {
        erp_ = erp;
        ApplyLimits();
        MarkNetworkUpdate();
    }
}

void Constraint::SetCFM(float cfm)
{
    cfm = Max(cfm, 0.0f);

    if (cfm != cfm_)
    {
        cfm_ = cfm;
        ApplyLimits();
        MarkNetworkUpdate();
    }
}

void Constraint::SetRelaxationFactor(float relaxationFactor)
{
    if (relaxationFactor_ != relaxationFactor)
    {
        relaxationFactor_ = relaxationFactor;

        if (constraint_->getConstraintType() != SLIDER_CONSTRAINT_TYPE)
        {
            ApplyLimits();
            MarkNetworkUpdate();
        }
    }
}

void Constraint::SetBiasFactor(float biasFactor)
{
    if (biasFactor_ != biasFactor)
    {
        biasFactor_ = biasFactor;

        if (constraint_->getConstraintType() != SLIDER_CONSTRAINT_TYPE)
        {
            ApplyLimits();
            MarkNetworkUpdate();
        }
    }
}

void Constraint::SetSoftness(float softness)
{
    if (softness_ != softness)
    {
        softness_ = softness;

        if (constraint_->getConstraintType() != SLIDER_CONSTRAINT_TYPE)
        {
            ApplyLimits();
            MarkNetworkUpdate();
        }
    }
}

void Constraint::SetDisableCollision(bool disable)
{
    if (disable != disableCollision_)
    {
        disableCollision_ = disable;
        CreateConstraint();
        MarkNetworkUpdate();
    }
}

Vector3 Constraint::GetWorldPosition() const
{
    if (constraint_)
    {
        btTransform ownBody = constraint_->getRigidBodyA().getWorldTransform();
        return ToVector3(ownBody * ToBtVector3(position_ * cachedWorldScale_ - ownBody_->GetCenterOfMass()));
    }
    else
        return Vector3::ZERO;
}

void Constraint::ReleaseConstraint()
{
    if (constraint_)
    {
        if (ownBody_)
            ownBody_->RemoveConstraint(this);
        if (otherBody_)
            otherBody_->RemoveConstraint(this);

        if (physicsWorld_)
            physicsWorld_->GetWorld()->removeConstraint(constraint_.Get());

        constraint_.Reset();
    }
}

void Constraint::ApplyFrames()
{
    if (!constraint_ || !node_ || (otherBody_ && !otherBody_->GetNode()))
        return;

    cachedWorldScale_ = node_->GetWorldScale();

    Vector3 ownBodyScaledPosition = position_ * cachedWorldScale_ - ownBody_->GetCenterOfMass();
    Vector3 otherBodyScaledPosition =
        otherBody_ ? otherPosition_ * otherBody_->GetNode()->GetWorldScale() - otherBody_->GetCenterOfMass() : otherPosition_;

    switch (constraint_->getConstraintType())
    {
    case POINT2POINT_CONSTRAINT_TYPE:
        {
            auto* pointConstraint = static_cast<btPoint2PointConstraint*>(constraint_.Get());
            pointConstraint->setPivotA(ToBtVector3(ownBodyScaledPosition));
            pointConstraint->setPivotB(ToBtVector3(otherBodyScaledPosition));
        }
        break;

    case HINGE_CONSTRAINT_TYPE:
        {
            auto* hingeConstraint = static_cast<btHingeConstraint*>(constraint_.Get());
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            hingeConstraint->setFrames(ownFrame, otherFrame);
        }
        break;

    case SLIDER_CONSTRAINT_TYPE:
        {
            auto* sliderConstraint = static_cast<btSliderConstraint*>(constraint_.Get());
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            sliderConstraint->setFrames(ownFrame, otherFrame);
        }
        break;

    case CONETWIST_CONSTRAINT_TYPE:
        {
            auto* coneTwistConstraint = static_cast<btConeTwistConstraint*>(constraint_.Get());
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            coneTwistConstraint->setFrames(ownFrame, otherFrame);
        }
        break;

    default:
        break;
    }
}

void Constraint::OnNodeSet(Node* node)
{
    if (node)
    {
        node->AddListener(this);
        cachedWorldScale_ = node->GetWorldScale();
    }
}

void Constraint::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        if (scene == node_)
            DRY_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        physicsWorld_ = scene->GetOrCreateComponent<PhysicsWorld>();
        physicsWorld_->AddConstraint(this);

        // Create constraint now if necessary (attributes modified before adding to scene)
        if (retryCreation_)
            CreateConstraint();
    }
    else
    {
        ReleaseConstraint();

        if (physicsWorld_)
            physicsWorld_->RemoveConstraint(this);

        // Recreate when moved to a scene again
        retryCreation_ = true;
    }
}

void Constraint::OnMarkedDirty(Node* node)
{
    /// \todo This does not catch the connected body node's scale changing
    if (HasWorldScaleChanged(cachedWorldScale_, node->GetWorldScale()))
        ApplyFrames();
}

void Constraint::CreateConstraint()
{
    DRY_PROFILE(CreateConstraint);

    cachedWorldScale_ = node_->GetWorldScale();

    ReleaseConstraint();

    ownBody_ = GetComponent<RigidBody>();
    btRigidBody* ownBody = ownBody_ ? ownBody_->GetBody() : nullptr;
    btRigidBody* otherBody = otherBody_ ? otherBody_->GetBody() : nullptr;

    // If no physics world available now mark for retry later
    if (!physicsWorld_ || !ownBody)
    {
        retryCreation_ = true;
        return;
    }

    if (!otherBody)
        otherBody = &btTypedConstraint::getFixedBody();

    Vector3 ownBodyScaledPosition = position_ * cachedWorldScale_ - ownBody_->GetCenterOfMass();
    Vector3 otherBodyScaledPosition = otherBody_ ? otherPosition_ * otherBody_->GetNode()->GetWorldScale() -
                                                   otherBody_->GetCenterOfMass() : otherPosition_;

    switch (constraintType_)
    {
    case CONSTRAINT_POINT:
        {
            constraint_ = new btPoint2PointConstraint(*ownBody, *otherBody, ToBtVector3(ownBodyScaledPosition),
                ToBtVector3(otherBodyScaledPosition));
        }
        break;

    case CONSTRAINT_HINGE:
        {
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            constraint_ = new btHingeConstraint(*ownBody, *otherBody, ownFrame, otherFrame);
        }
        break;

    case CONSTRAINT_SLIDER:
        {
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            constraint_ = new btSliderConstraint(*ownBody, *otherBody, ownFrame, otherFrame, false);
        }
        break;

    case CONSTRAINT_CONETWIST:
        {
            btTransform ownFrame(ToBtQuaternion(rotation_), ToBtVector3(ownBodyScaledPosition));
            btTransform otherFrame(ToBtQuaternion(otherRotation_), ToBtVector3(otherBodyScaledPosition));
            constraint_ = new btConeTwistConstraint(*ownBody, *otherBody, ownFrame, otherFrame);
        }
        break;

    default:
        break;
    }

    if (constraint_)
    {
        constraint_->setUserConstraintPtr(this);
        constraint_->setEnabled(IsEnabledEffective());
        ownBody_->AddConstraint(this);
        if (otherBody_)
            otherBody_->AddConstraint(this);

        ApplyLimits();

        physicsWorld_->GetWorld()->addConstraint(constraint_.Get(), disableCollision_);
    }

    recreateConstraint_ = false;
    framesDirty_ = false;
    retryCreation_ = false;
}

void Constraint::ApplyLimits()
{
    if (!constraint_)
        return;

    switch (constraint_->getConstraintType())
    {
    case HINGE_CONSTRAINT_TYPE:
        {
            auto* hingeConstraint = static_cast<btHingeConstraint*>(constraint_.Get());
            hingeConstraint->setLimit(lowLimit_.x_ * M_DEGTORAD, highLimit_.x_ * M_DEGTORAD, softness_, biasFactor_, relaxationFactor_);
        }
        break;

    case SLIDER_CONSTRAINT_TYPE:
        {
            auto* sliderConstraint = static_cast<btSliderConstraint*>(constraint_.Get());
            sliderConstraint->setUpperLinLimit(highLimit_.x_);
            sliderConstraint->setUpperAngLimit(highLimit_.y_ * M_DEGTORAD);
            sliderConstraint->setLowerLinLimit(lowLimit_.x_);
            sliderConstraint->setLowerAngLimit(lowLimit_.y_ * M_DEGTORAD);
        }
        break;

    case CONETWIST_CONSTRAINT_TYPE:
        {
            auto* coneTwistConstraint = static_cast<btConeTwistConstraint*>(constraint_.Get());
            coneTwistConstraint->setLimit(highLimit_.x_ * M_DEGTORAD, highLimit_.y_ * M_DEGTORAD, lowLimit_.x_ * M_DEGTORAD, softness_, biasFactor_, relaxationFactor_);
        }
        break;

    default:
        break;
    }

    if (erp_ != 0.0f)
        constraint_->setParam(BT_CONSTRAINT_STOP_ERP, erp_);
    if (cfm_ != 0.0f)
        constraint_->setParam(BT_CONSTRAINT_STOP_CFM, cfm_);
}

void Constraint::AdjustOtherBodyPosition()
{
    // Convenience for editing static constraints: if not connected to another body, adjust world position to match local
    // (when deserializing, the proper other body position will be read after own position, so this calculation is safely
    // overridden and does not accumulate constraint error
    if (constraint_ && !otherBody_)
    {
        btTransform ownBody = constraint_->getRigidBodyA().getWorldTransform();
        btVector3 worldPos = ownBody * ToBtVector3(position_ * cachedWorldScale_ - ownBody_->GetCenterOfMass());
        otherPosition_ = ToVector3(worldPos);
    }

    MarkFramesDirty();
}

}
