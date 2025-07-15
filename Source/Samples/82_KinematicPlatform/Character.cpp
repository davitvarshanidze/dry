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
#include <Dry/Graphics/AnimationController.h>
#include <Dry/Physics/KinematicCharacterController.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/IO/MemoryBuffer.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/SceneEvents.h>
#include <Dry/Math/Ray.h>
#include <Dry/IO/Log.h>

#include "Character.h"
#include "CollisionLayer.h"

#include <Dry/DebugNew.h>

const float WALK_FORCE = 3.f;
const float RUN_FORCE = 8.f;
const float INAIR_MOVE_FORCE = 15.f;
const float JUMP_FORCE = 13.f;
const float INAIR_THRESHOLD_TIME = 2.3f;
const Vector3 GRAVITY{ 0.f, -17.f, 0.f };

Character::Character(Context* context): LogicComponent(context),
    onGround_{ false },
    okToJump_{ true },
    inAirTimer_{ 0.0f },
    jumpStarted_{ false }
{
    SetUpdateEventMask(USE_UPDATE | USE_FIXEDUPDATE | USE_FIXEDPOSTUPDATE);
}

void Character::RegisterObject(Context* context)
{
    context->RegisterFactory<Character>();

    DRY_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
    DRY_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
    DRY_ATTRIBUTE("On Ground", bool, onGround_, false, AM_DEFAULT);
    DRY_ATTRIBUTE("OK To Jump", bool, okToJump_, true, AM_DEFAULT);
    DRY_ATTRIBUTE("In Air Timer", float, inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
    graphicsNode_ = node_->GetChild("Graphics");
    // Set a capsule shape for collision
    CollisionShape* shape = node_->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3::UP * .9f);

    kinematicController_ = node_->CreateComponent<KinematicCharacterController>();
    kinematicController_->SetCollisionLayerAndMask(ColLayer_Kinematic, ColMask_Kinematic);
    kinematicController_->SetStepHeight(1/3.f);
    kinematicController_->SetGravity(GRAVITY);
    kinematicController_->SetMaxSlope(40.f);

    // Component has been inserted into its scene node. Subscribe to events now
    SubscribeToEvent(GetNode(), E_NODECOLLISION, DRY_HANDLER(Character, HandleNodeCollision));
}

void Character::DelayedStart()
{
    collisionShape_ = node_->GetComponent<CollisionShape>(true);
    animController_ = node_->GetComponent<AnimationController>(true);
}

void Character::Update(float timeStep)
{
    // Gradually update node rotation based on movement speed and whether on ground
    node_->SetRotation(node_->GetRotation().Slerp(Quaternion(Vector3::FORWARD, curMoveDir_),
                                                  Min(1.f, timeStep * ((.23f + 4.2f * onGround_) * curMoveDir_.Length()))));
}

void Character::FixedUpdate(float timeStep)
{
    // Update the in air timer. Reset if grounded
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;
    // When character has been in air less than 1/10 second, it's still interpreted as being on ground
    bool softGrounded{ inAirTimer_ < INAIR_THRESHOLD_TIME };

    // Update movement & animation
    const Quaternion& rot{ controls_.yaw_, Vector3::UP };
    Vector3 moveDir{ Vector3::ZERO };
    onGround_ = kinematicController_->OnGround();

    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;

    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();

    // rotate movedir
    curMoveDir_ = rot * moveDir;

    const Vector3 lastWalkDir{ kinematicController_->GetLinearVelocity().ProjectOntoPlane(Vector3::UP) };
    kinematicController_->SetWalkDirection(lastWalkDir.Lerp(timeStep * curMoveDir_ * (
                                                                onGround_ ? (controls_.IsDown(CTRL_RUN) ? RUN_FORCE : WALK_FORCE)
                                                                          : INAIR_MOVE_FORCE),
                                                            Min(1.f, timeStep * 9.f * Max(0.f, (onGround_ ? 1.f : .05f) - PowN(inAirTimer_ / INAIR_THRESHOLD_TIME, 3)))));

    // Swimming
    const float waterLevel{ 20/3.f };


    PODVector<RayQueryResult> results{};
    Vector3 rayStart{ node_->GetWorldPosition().ProjectOntoPlane(Vector3::UP, Vector3::UP * (waterLevel + 1.f)) };
    RayOctreeQuery waterQuery{ results, { rayStart, Vector3::DOWN },
                               RAY_TRIANGLE, 1000.f, DRAWABLE_GEOMETRY, 128 };
    GetScene()->GetComponent<Octree>()->Raycast(waterQuery);

    bool foundWater{ false };
    for (RayQueryResult r: results)
//        Log::Write(LOG_INFO, r.node_->GetName());
        if (r.node_->HasTag("Water"))
            foundWater = true;


    if (node_->GetWorldPosition().y_ < waterLevel - onGround_ && foundWater)
    {

        const float submerged{ Pow(Clamp( .75f * (waterLevel - node_->GetWorldPosition().y_), 0.f, 1.f), 5.f) };
        kinematicController_->SetGravity(GRAVITY.Lerp(Vector3::UP * 0.0f, Min(1.f, submerged * 2.f)));
        kinematicController_->SetLinearDamping(submerged * 0.8f);

        kinematicController_->SetWalkDirection(lastWalkDir.Lerp(3.f * timeStep * curMoveDir_ * (controls_.IsDown(CTRL_RUN) ? RUN_FORCE : WALK_FORCE),
                                                                Min(1.f, timeStep * (submerged + .5f))));

        if (submerged > .5f)
        {
            kinematicController_->ApplyImpulse(timeStep * Min(0.f, .5f - submerged) * GRAVITY * .5f);
            okToJump_ = softGrounded = true;
        }

    }
    else
    {
        kinematicController_->SetGravity(GRAVITY);
        kinematicController_->SetLinearDamping(0.f);
    }

    if (softGrounded)
    {
        isJumping_ = false;
        // Jump. Must release jump control between jumps
        if (controls_.IsDown(CTRL_JUMP))
        {
            isJumping_ = true;

            if (okToJump_)
            {
                okToJump_ = false;
                jumpStarted_ = true;
                kinematicController_->Jump(kinematicController_->GetJumpSpeed() * (curMoveDir_+ Vector3::UP).Normalized());

                if (!Equals(curMoveDir_.LengthSquared(), 0.f))
                    node_->SetDirection(curMoveDir_);

                animController_->StopLayer(0);
                animController_->PlayExclusive("Ghotiland/Anim/Ozom/Jump.ani", 0, false, .05f);
                animController_->SetTime("Ghotiland/Anim/Ozom/Jump.ani", 0);
                animController_->SetSpeed("Ghotiland/Anim/Ozom/Jump.ani", 1.5f);
            }
        }
        else
        {
            okToJump_ = onGround_;
        }
    }

    if (!onGround_ || jumpStarted_)
    {
        if (jumpStarted_)
        {
            if (animController_->GetTime("Ghotiland/Anim/Ozom/Jump.ani") > .125f)
            {
                jumpStarted_ = false;
            }
        }
        else
        {
            const float maxDistance{ 50.0f };
            const float segmentDistance{ 10.01f };
            PhysicsRaycastResult result;
            GetScene()->GetComponent<PhysicsWorld>()->RaycastSingleSegmented(result,
                                                                             Ray{ node_->GetPosition(), Vector3::DOWN },
                                                                             maxDistance, segmentDistance, M_MAX_UNSIGNED);
            if (result.body_ && result.distance_ > 0.7f )
            {
                animController_->PlayExclusive("Ghotiland/Anim/Ozom/Jump.ani", 0, true, .2f);
                if (animController_->GetTime("Ghotiland/Anim/Ozom/Jump.ani") == .0f)
                    animController_->SetTime("Ghotiland/Anim/Ozom/Jump.ani", .5f);

                animController_->SetSpeed("Ghotiland/Anim/Ozom/Jump.ani", 1.75f - (animController_->GetTime("Ghotiland/Anim/Ozom/Jump.ani") - .125f));
            }
            //else if (result.body_ == NULL)
            //{
            //    // fall to death animation
            //}
        }
    }
    else
    {
        // Play walk animation if moving on ground, otherwise fade it out
        if (softGrounded && lastWalkDir.Length() > .01f)
        {
            animController_->PlayExclusive("Ghotiland/Anim/Ozom/Walk.ani", 0, true, .1f);
            animController_->SetSpeed("Ghotiland/Anim/Ozom/Walk.ani", .8f *  kinematicController_->GetLinearVelocity().Length() / timeStep);
        }
        else
        {
            animController_->PlayExclusive("Ghotiland/Anim/Ozom/Idle.ani", 0, true, 0.1f);
        }
    }
}

void Character::FixedPostUpdate(float timeStep)
{
    if (movingData_[0].IsSameNode(movingData_[1]))
    {
        Matrix3x4 delta{ movingData_[0].transform_ * movingData_[1].transform_.Inverse() };

        // Add delta
        Vector3 kPos;
        Quaternion kRot;
        kinematicController_->GetTransform(kPos, kRot);
        Matrix3x4 matKC{ kPos, kRot, Vector3::ONE };

        // Update
        matKC = delta * matKC;
        kinematicController_->SetTransform(matKC.Translation(), matKC.Rotation());

        // Update control and node yaw
        const float yaw{ delta.Rotation().YawAngle() };
        controls_.yaw_ += yaw;
        node_->Yaw(yaw);
    }

    // Update node position
    node_->SetWorldPosition(kinematicController_->GetPosition());

    // Shift and clear
    movingData_[1] = movingData_[0];
    movingData_[0].node_ = 0;
}

bool Character::IsNodeMovingPlatform(Node* node) const
{
    return node ? node->HasTag("MovingPlatform") : false;
}

void Character::NodeOnMovingPlatform(Node* node)
{
    if (!onGround_)
        return;

    if (!IsNodeMovingPlatform(node))
    {
        return;
    }

    movingData_[0].node_ = node;
    movingData_[0].transform_ = node->GetWorldTransform();
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;

    // possible moving platform trigger volume
    if (static_cast<RigidBody*>(eventData[P_OTHERBODY].GetVoidPtr())->IsTrigger())
        NodeOnMovingPlatform(static_cast<Node*>(eventData[P_OTHERNODE].GetVoidPtr()));
}
