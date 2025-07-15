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

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Physics/PhysicsUtils.h"
#include "../Physics/PhysicsWorld.h"
#include "../Physics/RaycastVehicle.h"
#include "../Physics/RigidBody.h"
#include "../Scene/Scene.h"

#include <Bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <Bullet/BulletDynamics/Vehicle/btRaycastVehicle.h>

namespace Dry
{

const IntVector3 RaycastVehicle::RIGHT_UP_FORWARD(0, 1, 2);
const IntVector3 RaycastVehicle::RIGHT_FORWARD_UP(0, 2, 1);
const IntVector3 RaycastVehicle::UP_FORWARD_RIGHT(1, 2, 0);
const IntVector3 RaycastVehicle::UP_RIGHT_FORWARD(1, 0, 2);
const IntVector3 RaycastVehicle::FORWARD_RIGHT_UP(2, 0, 1);
const IntVector3 RaycastVehicle::FORWARD_UP_RIGHT(2, 1, 0);

/// Raycast vehicle data.
struct RaycastVehicle::RaycastVehicleData
{
    /// Construct.
    RaycastVehicleData();
    /// Destruct.
    ~RaycastVehicleData();

    /// Return Bullet raycast vehicle.
    btRaycastVehicle* Get() const;

    /// Initialize vehicle data.
    void Init(Scene* scene, RigidBody* body, bool enabled, const IntVector3& coordinateSystem);
    /// Set the coordinate system.
    void SetCoordinateSystem(const IntVector3& coordinateSystem);
    /// Set enabled/disabled state.
    void SetEnabled(bool enabled);

    /// Physics world.
    WeakPtr<PhysicsWorld> physWorld_;
    /// Bullet raycast vehicle raycaster.
    btVehicleRaycaster* vehicleRayCaster_;
    /// Bullet raycast vehicle.
    btRaycastVehicle* vehicle_;
    /// Bullet raycast vehicle tuning.
    btRaycastVehicle::btVehicleTuning tuning_;
    /// Added flag.
    bool added_;
};

RaycastVehicle::RaycastVehicle(Context* context) :
    LogicComponent(context),
    activate_(false),
    hullBody_(nullptr),
    coordinateSystem_(RIGHT_UP_FORWARD),
    wheelNodes_(),
    origRotation_(),
    inAirRPM_(0.0f),
    skidInfoCumulative_(),
    wheelSideSlipSpeed_(),
    maxSideSlipSpeed_(4.0f),
    loadedWheelData_()
{
    vehicleData_ = new RaycastVehicleData();
    // FixedUpdate() for inputs and PostUpdate() to sync wheels for rendering
    SetUpdateEventMask(USE_FIXEDUPDATE | USE_FIXEDPOSTUPDATE | USE_POSTUPDATE);
}

RaycastVehicle::~RaycastVehicle()
{
    delete vehicleData_;
    wheelNodes_.Clear();
}

static const StringVector wheelElementNames =
{
    "Num Wheels",
    "Node Id",
    "Direction",
    "Axle",
    "Rest Length",
    "Radius",
    "Is Front Wheel",
    "Steering",
    "Connection Point Vector",
    "Original Rotation",
    "Cumulative Skid Info",
    "Side Skip Speed",
    "Is In Contact",
    "Contact Position",
    "Contact Normal",
    "Suspension Stiffness",
    "Max Suspension Force",
    "Damping Relaxation",
    "Damping Compression",
    "Friction Slip",
    "Roll Influence",
    "Engine Force",
    "Brake"
};

void RaycastVehicle::RegisterObject(Context* context)
{
    context->RegisterFactory<RaycastVehicle>();
    DRY_MIXED_ACCESSOR_ATTRIBUTE("Wheel Data", GetWheelDataAttr, SetWheelDataAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT)
        .SetMetadata(AttributeMetadata::P_VECTOR_STRUCT_ELEMENTS, wheelElementNames);
    DRY_ATTRIBUTE("Max Side Slip Speed", float, maxSideSlipSpeed_, 4.0f, AM_DEFAULT);
    DRY_ATTRIBUTE("In Air RPM", float, inAirRPM_, 0.0f, AM_DEFAULT);
    DRY_ATTRIBUTE("Coordinate System", IntVector3, coordinateSystem_, RIGHT_UP_FORWARD, AM_DEFAULT);
}

void RaycastVehicle::OnSetEnabled()
{
    if (vehicleData_)
        vehicleData_->SetEnabled(IsEnabledEffective());
}

void RaycastVehicle::ApplyAttributes()
{
    hullBody_ = node_->GetComponent<RigidBody>();
    Scene* scene = GetScene();

    if (!hullBody_ || !scene)
        return;

    vehicleData_->Init(scene, hullBody_, IsEnabledEffective(), coordinateSystem_);

    origRotation_.Clear();
    skidInfoCumulative_.Clear();
    wheelSideSlipSpeed_.Clear();

    if (loadedWheelData_.IsEmpty())
        return;

    int index = 0;
    const VariantVector& data = loadedWheelData_;
    const int numWheels = data[index++].GetInt();
    int wheelIndex = 0;

    for (int w{ 0 }; w < numWheels; w++)
    {
        const int node_id = data[index++].GetInt();
        const Vector3 direction = data[index++].GetVector3();
        const Vector3 axle = data[index++].GetVector3();
        const float restLength = data[index++].GetFloat();
        const float radius = data[index++].GetFloat();
        const bool isFrontWheel = data[index++].GetBool();
        const float steering = data[index++].GetFloat();
        const Vector3 connectionPoint = data[index++].GetVector3();
        const Quaternion origRotation = data[index++].GetQuaternion();
        const float skidInfoC = data[index++].GetFloat();
        const float sideSlipSpeed = data[index++].GetFloat();

        const bool isInContact = data[index++].GetBool();
        const Vector3 contactPosition = data[index++].GetVector3();
        const Vector3 contactNormal = data[index++].GetVector3();
        const float suspensionStiffness = data[index++].GetFloat();
        const float maxSuspensionForce = data[index++].GetFloat();
        const float dampingRelaxation = data[index++].GetFloat();
        const float dampingCompression = data[index++].GetFloat();
        const float frictionSlip = data[index++].GetFloat();
        const float rollInfluence = data[index++].GetFloat();
        const float engineForce = data[index++].GetFloat();
        const float brake = data[index++].GetFloat();
        const float skidInfo = data[index++].GetFloat();
        Node* wheelNode = GetScene()->GetNode(node_id);

        if (!wheelNode)
        {
            DRY_LOGERROR("RaycastVehicle: Incorrect node id = " + String(node_id) + " index: " + String(index));
            continue;
        }

        btRaycastVehicle* vehicle = vehicleData_->Get();
        const btVector3 connectionPointCS0(connectionPoint.x_, connectionPoint.y_, connectionPoint.z_);
        const btVector3 wheelDirectionCS0(direction.x_, direction.y_, direction.z_);
        const btVector3 wheelAxleCS(axle.x_, axle.y_, axle.z_);
        btWheelInfo& wheel = vehicle->addWheel(
                                 connectionPointCS0,
                                 wheelDirectionCS0,
                                 wheelAxleCS,
                                 restLength,
                                 radius,
                                 vehicleData_->tuning_,
                                 isFrontWheel);

        wheelNodes_.Push(wheelNode);
        origRotation_.Push(origRotation);
        skidInfoCumulative_.Push(skidInfoC);
        wheelSideSlipSpeed_.Push(sideSlipSpeed);
        SetSteeringValue(wheelIndex, steering);
        wheel.m_raycastInfo.m_isInContact = isInContact;
        wheel.m_raycastInfo.m_contactNormalWS = btVector3(contactNormal.x_, contactNormal.y_, contactNormal.z_);
        wheel.m_raycastInfo.m_contactPointWS = btVector3(contactPosition.x_, contactPosition.y_, contactPosition.z_);
        wheel.m_suspensionStiffness = suspensionStiffness;
        wheel.m_maxSuspensionForce = maxSuspensionForce;
        wheel.m_wheelsDampingRelaxation = dampingRelaxation;
        wheel.m_wheelsDampingCompression = dampingCompression;
        wheel.m_frictionSlip = frictionSlip;
        wheel.m_rollInfluence = rollInfluence;
        wheel.m_engineForce = engineForce;
        wheel.m_brake = brake;
        wheel.m_skidInfo = skidInfo;
        wheelIndex++;
    }

    DRY_LOGDEBUG("maxSideSlipSpeed_ value: " + String(maxSideSlipSpeed_));
    DRY_LOGDEBUG("loaded items: " + String(index));
    DRY_LOGDEBUG("loaded wheels: " + String(GetNumWheels()));
}

void RaycastVehicle::Init()
{
    hullBody_ = node_->GetOrCreateComponent<RigidBody>();
    Scene* scene = GetScene();
    vehicleData_->Init(scene, hullBody_, IsEnabledEffective(), coordinateSystem_);
}

void RaycastVehicle::FixedUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    if (!vehicle)
        return;

    for (int i{ 0 }; i < GetNumWheels(); i++)
    {
        btWheelInfo wInfo = vehicle->getWheelInfo(i);

        if (wInfo.m_engineForce != 0.0f || wInfo.m_steering != 0.0f)
        {
            hullBody_->Activate();
            break;
        }
    }
}

void RaycastVehicle::PostUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    if (!vehicle)
        return;

    for (int i{ 0 }; i < GetNumWheels(); i++)
    {
        vehicle->updateWheelTransform(i, true);
        const btTransform transform = vehicle->getWheelTransformWS(i);
        const Vector3 origin = ToVector3(transform.getOrigin());
        const Quaternion qRot = ToQuaternion(transform.getRotation());
        Node* pWheel = wheelNodes_[i];
        pWheel->SetWorldPosition(origin);
        pWheel->SetWorldRotation(qRot * origRotation_[i]);
    }
}

void RaycastVehicle::FixedPostUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    if (!vehicle || !hullBody_)
        return;

    const Vector3 velocity = hullBody_->GetLinearVelocity();

    for (int i{ 0 }; i < GetNumWheels(); i++)
    {
        btWheelInfo& wInfo = vehicle->getWheelInfo(i);

        if (!WheelIsInContact(i) && GetEngineForce(i) != 0.0f)
        {
            float delta;

            if (inAirRPM_ != 0.0f)
                delta = inAirRPM_ * timeStep / 60.0f;
            else
                delta = 8.0f * GetEngineForce(i) * timeStep / (hullBody_->GetMass() * GetWheelRadius(i));

            if (Abs(wInfo.m_deltaRotation) < Abs(delta))
            {
                wInfo.m_rotation += delta - wInfo.m_deltaRotation;
                wInfo.m_deltaRotation = delta;
            }

            if (skidInfoCumulative_[i] > 0.05f)
                skidInfoCumulative_[i] -= 0.002;
        }
        else
        {
            skidInfoCumulative_[i] = GetWheelSkidInfo(i);
        }

        wheelSideSlipSpeed_[i] = Abs(ToVector3(wInfo.m_raycastInfo.m_wheelAxleWS).DotProduct(velocity));

        if (wheelSideSlipSpeed_[i] > maxSideSlipSpeed_)
            skidInfoCumulative_[i] = Clamp(skidInfoCumulative_[i], 0.0f, 0.89f);
    }
}

void RaycastVehicle::SetMaxSideSlipSpeed(float speed)
{
    maxSideSlipSpeed_ = speed;
}

float RaycastVehicle::GetMaxSideSlipSpeed() const
{
    return maxSideSlipSpeed_;
}

void RaycastVehicle::SetWheelSkidInfoCumulative(int wheel, float skid)
{
    skidInfoCumulative_[wheel] = skid;
}

float RaycastVehicle::GetWheelSkidInfoCumulative(int wheel) const
{
    return skidInfoCumulative_[wheel];
}

void RaycastVehicle::AddWheel(Node* wheelNode, const Vector3& wheelDirection, const Vector3& wheelAxle,
                              float restLength, float wheelRadius, bool frontWheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const Vector3 connectionPoint = node_->WorldToLocal(wheelNode->GetWorldPosition());
    const btVector3 connectionPointCS0(ToBtVector3(connectionPoint));
    const btVector3 wheelDirectionCS0(ToBtVector3(wheelDirection));
    const btVector3 wheelAxleCS(ToBtVector3(wheelAxle));
    btWheelInfo& wheel = vehicle->addWheel(
                             connectionPointCS0,
                             wheelDirectionCS0,
                             wheelAxleCS,
                             restLength,
                             wheelRadius,
                             vehicleData_->tuning_,
                             frontWheel);

    wheelNodes_.Push(wheelNode);
    origRotation_.Push(wheelNode->GetWorldRotation());
    skidInfoCumulative_.Push(1.0f);
    wheelSideSlipSpeed_.Push(0.0f);
    wheel.m_raycastInfo.m_isInContact = false;
}

void RaycastVehicle::ResetSuspension()
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->resetSuspension();
}

void RaycastVehicle::UpdateWheelTransform(int wheel, bool interpolated)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->updateWheelTransform(wheel, interpolated);
}

Vector3 RaycastVehicle::GetWheelPosition(int wheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btTransform transform = vehicle->getWheelTransformWS(wheel);

    return ToVector3(transform.getOrigin());
}

Quaternion RaycastVehicle::GetWheelRotation(int wheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btTransform& transform = vehicle->getWheelTransformWS(wheel);

    return ToQuaternion(transform.getRotation());
}

Vector3 RaycastVehicle::GetWheelConnectionPoint(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return ToVector3(whInfo.m_chassisConnectionPointCS);
}

void RaycastVehicle::SetSteeringValue(int wheel, float steeringValue)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->setSteeringValue(steeringValue, wheel);
}

float RaycastVehicle::GetSteeringValue(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_steering;
}

void RaycastVehicle::SetWheelSuspensionStiffness(int wheel, float stiffness)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_suspensionStiffness = stiffness;
}

float RaycastVehicle::GetWheelSuspensionStiffness(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_suspensionStiffness;
}

void RaycastVehicle::SetWheelMaxSuspensionForce(int wheel, float force)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_maxSuspensionForce = force;
}

float RaycastVehicle::GetWheelMaxSuspensionForce(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_maxSuspensionForce;
}

void RaycastVehicle::SetWheelDampingRelaxation(int wheel, float damping)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_wheelsDampingRelaxation = damping;
}

float RaycastVehicle::GetWheelDampingRelaxation(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_wheelsDampingRelaxation;
}

void RaycastVehicle::SetWheelDampingCompression(int wheel, float compression)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_wheelsDampingCompression = compression;
}

float RaycastVehicle::GetWheelDampingCompression(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_wheelsDampingCompression;
}

void RaycastVehicle::SetWheelFrictionSlip(int wheel, float slip)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_frictionSlip = slip;
}

float RaycastVehicle::GetWheelFrictionSlip(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_frictionSlip;
}

void RaycastVehicle::SetWheelRollInfluence(int wheel, float rollInfluence)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_rollInfluence = rollInfluence;
}

Vector3 RaycastVehicle::GetContactPosition(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return ToVector3(whInfo.m_raycastInfo.m_contactPointWS);
}

Vector3 RaycastVehicle::GetContactNormal(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return ToVector3(whInfo.m_raycastInfo.m_contactNormalWS);
}

float RaycastVehicle::GetWheelSideSlipSpeed(int wheel) const
{
    return wheelSideSlipSpeed_[wheel];
}

float RaycastVehicle::GetWheelRollInfluence(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_rollInfluence;
}

void RaycastVehicle::SetWheelRadius(int wheel, float wheelRadius)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_wheelsRadius = wheelRadius;
}

float RaycastVehicle::GetWheelRadius(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_wheelsRadius;
}

void RaycastVehicle::SetEngineForce(int wheel, float force)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->applyEngineForce(force, wheel);
}

float RaycastVehicle::GetEngineForce(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_engineForce;
}

void RaycastVehicle::SetBrake(int wheel, float force)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->setBrake(force, wheel);
}

float RaycastVehicle::GetBrake(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_brake;
}

int RaycastVehicle::GetNumWheels() const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();

    return (vehicle ? vehicle->getNumWheels() : 0);
}

Node* RaycastVehicle::GetWheelNode(int wheel) const
{
    return wheelNodes_[wheel];
}

void RaycastVehicle::SetMaxSuspensionTravel(int wheel, float maxSuspensionTravel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_maxSuspensionTravelCm = maxSuspensionTravel;
}

float RaycastVehicle::GetMaxSuspensionTravel(int wheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_maxSuspensionTravelCm;
}

void RaycastVehicle::SetWheelDirection(int wheel, const Vector3& direction)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_wheelDirectionCS = ToBtVector3(direction);
}

Vector3 RaycastVehicle::GetWheelDirection(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return ToVector3(whInfo.m_wheelDirectionCS);
}

void RaycastVehicle::SetWheelAxle(int wheel, const Vector3& axle)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_wheelAxleCS = ToBtVector3(axle);
}

Vector3 RaycastVehicle::GetWheelAxle(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return ToVector3(whInfo.m_wheelAxleCS);
}

void RaycastVehicle::SetWheelRestLength(int wheel, float length)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_suspensionRestLength1 = length;
}

float RaycastVehicle::GetWheelRestLength(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_suspensionRestLength1;
}

void RaycastVehicle::SetWheelSkidInfo(int wheel, float factor)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);
    whInfo.m_skidInfo = factor;
}

float RaycastVehicle::GetWheelSkidInfo(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_skidInfo;
}

bool RaycastVehicle::IsFrontWheel(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo& whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_bIsFrontWheel;
}

bool RaycastVehicle::WheelIsInContact(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btWheelInfo whInfo = vehicle->getWheelInfo(wheel);

    return whInfo.m_raycastInfo.m_isInContact;
}

void RaycastVehicle::SetInAirRPM(float rpm)
{
    inAirRPM_ = rpm;
}

float RaycastVehicle::GetInAirRPM() const
{
    return inAirRPM_;
}

void RaycastVehicle::SetCoordinateSystem(const IntVector3& coordinateSystem)
{
    coordinateSystem_ = coordinateSystem;
    vehicleData_->SetCoordinateSystem(coordinateSystem_);
}

void RaycastVehicle::ResetWheels()
{
    ResetSuspension();

    for (int i{ 0 }; i < GetNumWheels(); i++)
    {
        UpdateWheelTransform(i, true);
        const Vector3 origin = GetWheelPosition(i);
        Node* wheelNode = GetWheelNode(i);
        wheelNode->SetWorldPosition(origin);
    }
}

VariantVector RaycastVehicle::GetWheelDataAttr() const
{
    const int numWheels{ GetNumWheels() };
    VariantVector ret;
    ret.Reserve(1 + 22 * numWheels);
    ret.Push(numWheels);

    for (int i{ 0 }; i < numWheels; i++)
    {
        Node* wNode = GetWheelNode(i);
        int node_id = wNode->GetID();
        DRY_LOGDEBUG("RaycastVehicle: Saving node id = " + String(node_id));

        ret.Push(node_id);
        ret.Push(GetWheelDirection(i));
        ret.Push(GetWheelAxle(i));
        ret.Push(GetWheelRestLength(i));
        ret.Push(GetWheelRadius(i));
        ret.Push(IsFrontWheel(i));
        ret.Push(GetSteeringValue(i));
        ret.Push(GetWheelConnectionPoint(i));
        ret.Push(origRotation_[i]);
        ret.Push(GetWheelSkidInfoCumulative(i));
        ret.Push(GetWheelSideSlipSpeed(i));
        ret.Push(WheelIsInContact(i));
        ret.Push(GetContactPosition(i));
        ret.Push(GetContactNormal(i));       // 14
        ret.Push(GetWheelSuspensionStiffness(i));
        ret.Push(GetWheelMaxSuspensionForce(i));
        ret.Push(GetWheelDampingRelaxation(i));
        ret.Push(GetWheelDampingCompression(i));
        ret.Push(GetWheelFrictionSlip(i));
        ret.Push(GetWheelRollInfluence(i));
        ret.Push(GetEngineForce(i));
        ret.Push(GetBrake(i));
        ret.Push(GetWheelSkidInfo(i));
    }

    DRY_LOGDEBUG("RaycastVehicle: saved items: " + String(ret.Size()));
    DRY_LOGDEBUG("maxSideSlipSpeed_ value save: " + String(maxSideSlipSpeed_));

    return ret;
}

void RaycastVehicle::SetWheelDataAttr(const VariantVector& value)
{
    if (!vehicleData_)
    {
        DRY_LOGERROR("RaycastVehicle: Vehicle data does not exist");
        return;
    }

    if (value.IsEmpty())
        DRY_LOGERROR("RaycastVehicle: Incorrect vehicle data");

    loadedWheelData_ = value;
}

void RaycastVehicle::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (int w{ 0 }; w < GetNumWheels(); ++w)
    {
        const Quaternion nodeRot{ node_->GetWorldRotation() };
        const Vector3 connectionPoint{ node_->LocalToWorld(hullBody_->GetCenterOfMass() + GetWheelConnectionPoint(w)) };
        const Vector3 wheelPosition{ GetWheelPosition(w) };
        debug->AddLine(connectionPoint, wheelPosition, Color::RED, depthTest);
        debug->AddLine(connectionPoint, connectionPoint - nodeRot * GetWheelDirection(w) * .25f * GetWheelRestLength(w), Color::BLUE, depthTest);
        debug->AddCircle(wheelPosition,
                         GetWheelRotation(w) * GetWheelAxle(w),
                         GetWheelRadius(w),
                         Color::WHITE, 128, depthTest);
    }
}

RaycastVehicle::RaycastVehicleData::RaycastVehicleData():
    physWorld_(nullptr),
    vehicleRayCaster_(nullptr),
    vehicle_(nullptr),
    tuning_(),
    added_(false)
{
}

RaycastVehicle::RaycastVehicleData::~RaycastVehicleData()
{
    if (vehicleRayCaster_)
    {
        delete vehicleRayCaster_;
        vehicleRayCaster_ = nullptr;
    }

    if (vehicle_)
    {
        if (physWorld_ && added_)
        {
            btDynamicsWorld* pbtDynWorld = physWorld_->GetWorld();

            if (pbtDynWorld)
                pbtDynWorld->removeAction(vehicle_);

            added_ = false;
        }

        delete vehicle_;
        vehicle_ = nullptr;
    }
}

btRaycastVehicle* RaycastVehicle::RaycastVehicleData::Get() const { return vehicle_; }

void RaycastVehicle::RaycastVehicleData::Init(Scene* scene, RigidBody* body, bool enabled, const IntVector3& coordinateSystem)
{
    PhysicsWorld* pPhysWorld = scene->GetComponent<PhysicsWorld>();
    btDynamicsWorld* pbtDynWorld = pPhysWorld->GetWorld();

    if (!pbtDynWorld)
        return;

    // Delete old vehicle & action first
    delete vehicleRayCaster_;

    if (vehicle_)
    {
        if (added_)
            pbtDynWorld->removeAction(vehicle_);

        delete vehicle_;
    }

    vehicleRayCaster_ = new btDefaultVehicleRaycaster(pbtDynWorld);
    btRigidBody* bthullBody = body->GetBody();
    vehicle_ = new btRaycastVehicle(tuning_, bthullBody, vehicleRayCaster_);

    if (enabled)
    {
        pbtDynWorld->addAction(vehicle_);
        added_ = true;
    }

    SetCoordinateSystem(coordinateSystem);
    physWorld_ = pPhysWorld;
}

void RaycastVehicle::RaycastVehicleData::SetCoordinateSystem(const IntVector3& coordinateSystem)
{
    if (vehicle_)
        vehicle_->setCoordinateSystem(coordinateSystem.x_, coordinateSystem.y_, coordinateSystem.z_);
}

void RaycastVehicle::RaycastVehicleData::SetEnabled(bool enabled)
{
    if (!physWorld_ || !vehicle_)
        return;

    btDynamicsWorld* pbtDynWorld = physWorld_->GetWorld();
    if (!pbtDynWorld)
        return;

    if (enabled && !added_)
    {
        pbtDynWorld->addAction(vehicle_);
        added_ = true;
    }
    else if (!enabled && added_)
    {
        pbtDynWorld->removeAction(vehicle_);
        added_ = false;
    }
}

}
