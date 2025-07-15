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

#include "Vehicle.h"
#include <Dry/Core/Context.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/DecalSet.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/ParticleEffect.h>
#include <Dry/Graphics/ParticleEmitter.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/IO/Log.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/Constraint.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RaycastVehicle.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>

using namespace Dry;

const float CHASSIS_WIDTH{ 2.6f };

void Vehicle::RegisterObject(Context* context)
{
    context->RegisterFactory<Vehicle>();
    DRY_ATTRIBUTE("Steering", float, steering_, 0.f, AM_DEFAULT);
    DRY_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.f, AM_DEFAULT);
    DRY_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 23.0f, AM_DEFAULT);
}

Vehicle::Vehicle(Dry::Context* context): LogicComponent(context),
    controls_{},
    steering_{ 0.f }
{
    controls_.pitch_ = 23.f;

    SetUpdateEventMask(USE_FIXEDUPDATE | USE_POSTUPDATE);
    engineForce_ = 0.f;
    brakingForce_ = 100.f;
    vehicleSteering_ = 0.f;
    maxEngineForce_ = 2500.f;
    wheelRadius_ = 0.5f;
    suspensionRestLength_ = 0.6f;
    wheelWidth_ = 0.4f;
    suspensionStiffness_ = 14.0f;
    suspensionDamping_ = 2.0f;
    suspensionCompression_ = 4.0f;
    wheelFriction_ = 1000.f;
    rollInfluence_ = 0.12f;
    emittersCreated_ = false;
}

Vehicle::~Vehicle() = default;

void Vehicle::Init()
{
    // This function is called only from the main program when initially creating the vehicle, not on scene load
    RaycastVehicle* vehicle{ node_->CreateComponent<RaycastVehicle>() };
    vehicle->Init();

    RigidBody* hullBody{ node_->GetComponent<RigidBody>() };
    hullBody->SetMass(800.f);
    hullBody->SetLinearDamping(.1f); // Some air resistance
    hullBody->SetAngularDamping(.1f);
    hullBody->SetCollisionLayer(1);
    hullBody->SetRestitution(.1f);
    hullBody->SetFriction(.5f);
    hullBody->SetRollingFriction(0.f);
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    // Setting-up collision shape
    CollisionShape* hullColShape{ node_->CreateComponent<CollisionShape>() };
    Vector3 chassisScale{ CHASSIS_WIDTH, 1.f, 5.f };
    hullColShape->SetMargin(.075f);
    hullColShape->SetBox(chassisScale - Vector3::ONE * 2.f * hullColShape->GetMargin(), Vector3::DOWN * .25f);
    hullBody->UpdateMass();
    Node* chassisNode{ node_->CreateChild("Chassis") };
    chassisNode->SetScale(hullColShape->GetSize() + 2.f * Vector3::ONE * hullColShape->GetMargin());
    chassisNode->SetPosition(hullColShape->GetPosition());
    StaticModel* hullObject{ chassisNode->CreateComponent<StaticModel>() };
    hullObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    hullObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    hullObject->SetCastShadows(true);
    float connectionHeight{ -.5f * wheelRadius_ };
    bool isFrontWheel{ true };
    const Vector3 wheelDirection{ 0.f, -1.f, 0.f };
    const Vector3 wheelAxle{ -1.f, 0.f, 0.f };

    const float wheelX{ .5f * CHASSIS_WIDTH };
    const float wheelZ{ .5f * chassisScale.z_ - wheelRadius_ };
    // Front left
    connectionPoints_[0] = { -wheelX, connectionHeight,  wheelZ };
    // Front right
    connectionPoints_[1] = {  wheelX, connectionHeight,  wheelZ };
    // Back left
    connectionPoints_[2] = { -wheelX, connectionHeight, -wheelZ };
    // Back right
    connectionPoints_[3] = {  wheelX, connectionHeight, -wheelZ };

    for (int id{ 0 }; id < sizeof(connectionPoints_) / sizeof(connectionPoints_[0]); id++)
    {
        Node* wheelNode{ node_->CreateChild() };
        Vector3 connectionPoint{ connectionPoints_[id] };
        // Front wheels are at front (z > 0)
        // back wheels are at z < 0
        // Setting rotation according to wheel position
        isFrontWheel = connectionPoints_[id].z_ > 0.f;
        wheelNode->SetRotation(connectionPoint.x_ >= 0.f ? Quaternion{ 0.f, 0.f, -90.f } : Quaternion{ 0.f, 0.f, 90.f });
        wheelNode->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * connectionPoints_[id]);
        vehicle->AddWheel(wheelNode, wheelDirection, wheelAxle, suspensionRestLength_, wheelRadius_, isFrontWheel);
        vehicle->SetWheelSuspensionStiffness(id, suspensionStiffness_);
        vehicle->SetWheelDampingRelaxation(id, suspensionDamping_);
        vehicle->SetWheelDampingCompression(id, suspensionCompression_);
        vehicle->SetWheelFrictionSlip(id, wheelFriction_);
        vehicle->SetWheelRollInfluence(id, rollInfluence_);
        const float wheelDiameter{ 2.f * wheelRadius_ };
        wheelNode->SetScale({ wheelDiameter, wheelWidth_, wheelDiameter });
        StaticModel* pWheel{ wheelNode->CreateComponent<StaticModel>() };
        pWheel->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
        pWheel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        pWheel->SetCastShadows(true);
        CreateEmitter(connectionPoints_[id]);
    }

    emittersCreated_ = true;
    vehicle->ResetWheels();
}

void Vehicle::CreateEmitter(const Vector3& position)
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Node* dustNode{ GetScene()->CreateChild() };
    dustNode->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * position + Vector3{ 0.f, -wheelRadius_, 0.f });
    ParticleEmitter* dustEmitter{ dustNode->CreateComponent<ParticleEmitter>() };
    dustEmitter->SetEffect(cache->GetResource<ParticleEffect>("Particle/Dust.xml"));
    dustEmitter->SetEmitting(false);
    particleEmitterNodeList_.Push(dustNode);
    dustNode->SetTemporary(true);
}

void Vehicle::ApplyAttributes()
{
    RaycastVehicle* vehicle{ node_->GetComponent<RaycastVehicle>() };
    if (!vehicle || emittersCreated_)
        return;

    for (const auto& connectionPoint: connectionPoints_)
        CreateEmitter(connectionPoint);

    emittersCreated_ = true;
}

void Vehicle::FixedUpdate(float timeStep)
{
    RaycastVehicle* vehicle{ node_->GetComponent<RaycastVehicle>() };
    if (!vehicle)
        return;

    float newSteering{ 0.f };
    float accelerator{ 0.f };
    bool brake{ false };

    // Read controls
    if (controls_.buttons_ & CTRL_LEFT)
        newSteering -= 1.f;

    if (controls_.buttons_ & CTRL_RIGHT)
        newSteering += 1.f;

    if (controls_.buttons_ & CTRL_FORWARD)
        accelerator = 1.f;

    if (controls_.buttons_ & CTRL_BACK)
    {
        if (accelerator == 0.f)
            accelerator = -.5f;
        else
            accelerator = 0.f;
    }

    if (controls_.buttons_ & CTRL_BRAKE)
        brake = true;

    // When steering, wake up the wheel rigidbodies so that their orientation is updated
    if (newSteering != 0.f)
        SetSteering(Lerp(GetSteering(), newSteering, timeStep * 2.f));
    else
        SetSteering(Lerp(GetSteering(), newSteering, timeStep * 16.f));

    // Set front wheel angles
    vehicleSteering_ = steering_;
    int wheelIndex{ 0 };
    vehicle->SetSteeringValue(wheelIndex, vehicleSteering_ * .5f);
    wheelIndex = 1;
    vehicle->SetSteeringValue(wheelIndex, vehicleSteering_ * .5f);
    // apply forces
    engineForce_ = maxEngineForce_ * accelerator;
    // 2x wheel drive
    vehicle->SetEngineForce(2, engineForce_);
    vehicle->SetEngineForce(3, engineForce_);

    for (int i{ 0 }; i < vehicle->GetNumWheels(); ++i)
    {
        if (brake)
            vehicle->SetBrake(i, !vehicle->IsFrontWheel(i) * brakingForce_);
        else
            vehicle->SetBrake(i, 0.f);
    }
}

void Vehicle::PostUpdate(float timeStep)
{
    RaycastVehicle* vehicle{ node_->GetComponent<RaycastVehicle>() };
    if (!vehicle)
        return;

    RigidBody* vehicleBody{ node_->GetComponent<RigidBody>() };
    const Vector3 velocity{ vehicleBody->GetLinearVelocity() };
    const Vector3 acceleration{ (velocity - prevVelocity_) / timeStep };
    const float planeAcceleration{ acceleration.ProjectOntoPlane(Vector3::UP).Length() };

    for (int i{ 0 }; i < vehicle->GetNumWheels(); i++)
    {
        Node* emitter{ particleEmitterNodeList_[i] };
        ParticleEmitter* particleEmitter{ emitter->GetComponent<ParticleEmitter>() };

        if (vehicle->WheelIsInContact(i) && (vehicle->GetWheelSkidInfoCumulative(i) < .9f || vehicle->GetBrake(i) > 2.f ||
            planeAcceleration > 15.f))
        {
            particleEmitterNodeList_[i]->SetWorldPosition(vehicle->GetContactPosition(i));

            if (!particleEmitter->IsEmitting())
                particleEmitter->SetEmitting(true);

            DRY_LOGDEBUG("GetWheelSkidInfoCumulative() = " +
                            String{ vehicle->GetWheelSkidInfoCumulative(i) } + " " +
                            String{ vehicle->GetMaxSideSlipSpeed() });
            /* TODO: Add skid marks here */
        }
        else if (particleEmitter->IsEmitting())
        {
            particleEmitter->SetEmitting(false);
        }
    }

    prevVelocity_ = velocity;
}
