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

#include <Dry/Core/Context.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/PhysicsUtils.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/SceneEvents.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/Scene/ObjectAnimation.h>
#include <SDL/SDL_log.h>

#include "MovingPlatform.h"
#include "Character.h"

#include <Dry/DebugNew.h>


MovingPlatform::MovingPlatform(Context* context): LogicComponent(context),
    speed_{ .042f },
    sway_{ { 0.f, 42.f }, Vector2::UP, PT_HARMONIC_SIN }
{
    sway_.SetSlope({ .5f, speed_ / M_PI});
    SetUpdateEventMask(USE_NO_EVENT);
}

MovingPlatform::~MovingPlatform()
{
}

void MovingPlatform::RegisterObject(Context* context)
{
    context->RegisterFactory<MovingPlatform>();
}

void MovingPlatform::Initialize(Node* platformNode, const Vector3& finishPosition, bool updateBodyOnPlatform)
{
    // get other lift components
    platformNode_ = platformNode;
    platformVolumdNode_ = platformNode_->GetChild("Volume", true);

    assert( platformNode_ && platformVolumdNode_ && "missing nodes!" );

    // positions
    initialPosition_   = platformNode_->GetWorldPosition();
    finishPosition_    = finishPosition;

    movement_ = new ValueAnimation{ context_ };
    movement_->SetInterpolationMethod(IM_SPLINE);

    movement_->SetKeyFrame(0.f, initialPosition_);
    movement_->SetKeyFrame(.6f, finishPosition_);
    movement_->SetKeyFrame(1.f, initialPosition_);

    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void MovingPlatform::FixedUpdate(float timeStep)
{
    platformNode_->SetPosition(movement_->GetAnimationValue(speed_ * GetScene()->GetElapsedTime(), WM_LOOP).GetVector3());

    platformNode_->SetRotation({ sway_.Solve(GetScene()->GetElapsedTime()), Vector3::UP });
}




