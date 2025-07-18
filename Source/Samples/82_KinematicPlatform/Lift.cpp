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
#include <Dry/Physics/RigidBody.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/SceneEvents.h>
#include <SDL/SDL_log.h>

#include "Lift.h"

#include <Dry/DebugNew.h>


Lift::Lift(Context* context): LogicComponent(context),
    liftState_{ LIFT_STATE_START },
    liftButtonState_{ LIFT_BUTTON_UP },
    maxLiftSpeed_{ 5.0f },
    minLiftSpeed_{ 1.5f },
    curLiftSpeed_{ 0.0f },
    buttonPressed_{ false },
    buttonPressedHeight_{ .25f },
    standingOnButton_{ false }
{
    SetUpdateEventMask(USE_NO_EVENT);
}

Lift::~Lift()
{
}

void Lift::RegisterObject(Context* context)
{
    context->RegisterFactory<Lift>();
}

void Lift::Start()
{
}

void Lift::Initialize(Node* liftNode, const Vector3& finishPosition)
{
    // get other lift components
    liftNode_        = liftNode;
    liftButtonNode_  = liftNode_->GetChild("LiftButton", true);

    assert( liftNode_ && liftButtonNode_ && "missing nodes!" );

    // positions
    initialPosition_   = liftNode_->GetWorldPosition();
    finishPosition_    = finishPosition;
    directionToFinish_ = (finishPosition_ - initialPosition_).Normalized();
    totalDistance_     = (finishPosition_ - initialPosition_).Length();

    // events
    SubscribeToEvent(liftButtonNode_, E_NODECOLLISIONSTART, DRY_HANDLER(Lift, HandleButtonStartCollision));
    SubscribeToEvent(liftButtonNode_, E_NODECOLLISIONEND, DRY_HANDLER(Lift, HandleButtonEndCollision));
}

void Lift::FixedUpdate(float timeStep)
{
    Vector3 liftPos = liftNode_->GetWorldPosition();
    Vector3 newPos = liftPos;

    // move lift
    if (liftState_ == LIFT_STATE_MOVETO_FINISH)
    {
        const Vector3 curDistance{ finishPosition_ - liftPos };
        const Vector3 curDirection{ curDistance.Normalized() };
        const float dist{ curDistance.Length() };
        const float dotd{ directionToFinish_.DotProduct(curDirection) };

        if (dotd > 0.0f)
        {
            // slow down near the end
            if (dist < 1.0f)
            {
                curLiftSpeed_ *= 0.92f;
            }
            curLiftSpeed_ = Clamp(curLiftSpeed_, minLiftSpeed_, maxLiftSpeed_);
            newPos += curDirection * curLiftSpeed_ * timeStep;
        }
        else
        {
            newPos = finishPosition_;
            SetTransitionCompleted(LIFT_STATE_FINISH);
        }
        liftNode_->SetWorldPosition(newPos);
    }
    else if (liftState_ == LIFT_STATE_MOVETO_START)
    {
        Vector3 curDistance  = initialPosition_ - liftPos;
        Vector3 curDirection = curDistance.Normalized();
        float dist = curDistance.Length();
        float dotd = directionToFinish_.DotProduct(curDirection);

        if (dotd < 0.0f)
        {
            // slow down near the end
            if (dist < 1.0f)
            {
                curLiftSpeed_ *= 0.92f;
            }
            curLiftSpeed_ = Clamp(curLiftSpeed_, minLiftSpeed_, maxLiftSpeed_);
            newPos += curDirection * curLiftSpeed_ * timeStep;
        }
        else
        {
            newPos = initialPosition_;
            SetTransitionCompleted(LIFT_STATE_START);
        }

        liftNode_->SetWorldPosition(newPos);
    }

    // reenable button
    if (!standingOnButton_ && 
        liftButtonState_ == LIFT_BUTTON_DOWN &&
        (liftState_ == LIFT_STATE_START || liftState_ == LIFT_STATE_FINISH))
    {
        liftButtonState_ = LIFT_BUTTON_UP;
        ButtonPressAnimate(false);
    }
}

void Lift::SetTransitionCompleted(int toState)
{
    liftState_ = toState;

    // adjust button
    if (liftButtonState_ == LIFT_BUTTON_UP)
    {
        ButtonPressAnimate(false);
    }
}

void Lift::ButtonPressAnimate(bool pressed)
{
    Node* node{ liftButtonNode_->GetChild("GraphicsNode") };

    if (pressed)
    {
        node->SetPosition(Vector3::DOWN * buttonPressedHeight_);
        buttonPressed_ = true;
    }
    else
    {
        node->SetPosition(Vector3::ZERO);
        buttonPressed_ = false;
    }
}

void Lift::HandleButtonStartCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;

    standingOnButton_ = true;

    if (liftButtonState_ == LIFT_BUTTON_UP)
    {
        if (liftState_ == LIFT_STATE_START)
        {
            liftState_ = LIFT_STATE_MOVETO_FINISH;
            liftButtonState_ = LIFT_BUTTON_DOWN;
            curLiftSpeed_ = maxLiftSpeed_;

            // Adjust button
            ButtonPressAnimate(true);

            SetUpdateEventMask(USE_FIXEDUPDATE);
        }
        else if (liftState_ == LIFT_STATE_FINISH)
        {
            liftState_ = LIFT_STATE_MOVETO_START;
            liftButtonState_ = LIFT_BUTTON_DOWN;
            curLiftSpeed_ = maxLiftSpeed_;

            // Adjust button
            ButtonPressAnimate(true);

            SetUpdateEventMask(USE_FIXEDUPDATE);
        }
    
        // play sound and animation
    }
}

void Lift::HandleButtonEndCollision(StringHash eventType, VariantMap& eventData)
{
    standingOnButton_ = false;

    if (liftButtonState_ == LIFT_BUTTON_DOWN)
    {
        // button animation
        if (liftState_ == LIFT_STATE_START || liftState_ == LIFT_STATE_FINISH)
        {
            liftButtonState_ = LIFT_BUTTON_UP;
            ButtonPressAnimate(false);
        }
    }
}



