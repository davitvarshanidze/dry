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

#include <Dry/Input/Controls.h>
#include <Dry/Scene/LogicComponent.h>

namespace Dry
{

class AnimationController;
class RigidBody;
class CollisionShape;

}

using namespace Dry;
class KinematicCharacterController;

const int CTRL_FORWARD = 1;
const int CTRL_BACK = 2;
const int CTRL_LEFT = 4;
const int CTRL_RIGHT = 8;
const int CTRL_JUMP = 16;
const int CTRL_RUN = 32;
const float YAW_SENSITIVITY = 0.23f;

struct MovingData
{
    MovingData():
        node_{ nullptr }
    {
    }
    
    MovingData& operator =(const MovingData& rhs)
    {
        node_ = rhs.node_;
        transform_ = rhs.transform_;
        return *this;
    }

    bool IsSameNode(const MovingData& rhs)
    {
        return (node_ && node_ == rhs.node_);
    }

    Node* node_;
    Matrix3x4 transform_;
};

/// Character component, responsible for physical movement according to controls, as well as animation.
class Character: public LogicComponent
{
    DRY_OBJECT(Character, LogicComponent);

public:
    /// Construct.
    Character(Context* context);
    
    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle delayed start.
    virtual void DelayedStart();

    /// Handle startup. Called by LogicComponent base class.
    virtual void Start();
    /// Handle update.
    void Update(float timeStep) override;
    /// Handle physics world update. Called by LogicComponent base class.
    void FixedUpdate(float timeStep) override;
    /// Handle physics world post update.
    void FixedPostUpdate(float timeStep) override;
    
    void SetOnMovingPlatform(RigidBody* platformBody)
    { 
        //onMovingPlatform_ = (platformBody != NULL);
        //platformBody_ = platformBody; 
    }

public:
    /// Movement controls. Assigned by the main program each frame.
    Controls controls_;

protected:
    bool IsNodeMovingPlatform(Node* node) const;
    void NodeOnMovingPlatform(Node* node);
    /// Handle physics collision event.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
    
protected:
    Node* graphicsNode_;
    bool jumper_;
    bool swimmer_;
    bool flyer_;

    bool onGround_;
    bool okToJump_;
    float inAirTimer_;

    // extra vars
    Vector3 curMoveDir_;
    bool isJumping_;
    bool jumpStarted_;

    WeakPtr<CollisionShape> collisionShape_;
    WeakPtr<AnimationController> animController_;
    WeakPtr<KinematicCharacterController> kinematicController_;

    // moving platform data
    MovingData movingData_[2];
};
