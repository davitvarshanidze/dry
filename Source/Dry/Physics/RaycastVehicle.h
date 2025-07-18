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

/// \file

#pragma once

#include "../Scene/LogicComponent.h"
#include "../Physics/PhysicsUtils.h"
#include "../Physics/RigidBody.h"

namespace Dry
{

class DRY_API RaycastVehicle : public LogicComponent
{
    DRY_OBJECT(RaycastVehicle, LogicComponent)

public:
    /// Construct.
    explicit RaycastVehicle(Dry::Context* context);
    /// Destruct.
    ~RaycastVehicle() override;

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Perform post-load after deserialization. Acquire the components from the scene nodes.
    void ApplyAttributes() override;

    /// Add a wheel. All parameters are relative to RigidBody / node.
    void AddWheel(Node* wheelNode, const Vector3& wheelDirection, const Vector3& wheelAxle, float restLength, float wheelRadius, bool frontWheel);
    /// Reset all suspension.
    void ResetSuspension();
    /// Update transform for particular wheel.
    void UpdateWheelTransform(int wheel, bool interpolated);
    /// Set steering value of particular wheel.
    void SetSteeringValue(int wheel, float steeringValue);
    /// Set suspension stiffness for particular wheel.
    void SetWheelSuspensionStiffness(int wheel, float stiffness);
    /// Set wheel max suspension force. Good results are often obtained by a value that is about 3x to 4x the vehicle weight. 
    void SetWheelMaxSuspensionForce(int wheel, float force);
    /// Set wheel damping relaxation.
    void SetWheelDampingRelaxation(int wheel, float damping);
    /// Set wheel damping compression.
    void SetWheelDampingCompression(int wheel, float compression);
    /// Set wheel friction slip.
    void SetWheelFrictionSlip(int wheel, float slip);
    /// Set wheel roll influence.
    void SetWheelRollInfluence(int wheel, float rollInfluence);
    /// Set engine force for the wheel.
    void SetEngineForce(int wheel, float force);
    /// Set hand brake (wheel rotation blocking force).
    void SetBrake(int wheel, float force);
    /// Set wheel radius.
    void SetWheelRadius(int wheel, float wheelRadius);
    /// Sets node initial positions.
    void ResetWheels();
    /// Set sliding factor 0 <= x <= 1. The less the value, more sliding.
    void SetWheelSkidInfo(int wheel, float factor);
    /// True if wheel touches ground (raycast hits something).
    bool WheelIsInContact(int wheel) const;
    /// Set maximum suspension travel value.
    void SetMaxSuspensionTravel(int wheel, float maxSuspensionTravel);
    /// Set wheel direction vector.
    void SetWheelDirection(int wheel, const Vector3& direction);
    /// Set wheel axle vector.
    void SetWheelAxle(int wheel, const Vector3& axle);
    /// Set side speed which is considered sliding.
    void SetMaxSideSlipSpeed(float speed);
    /// Set cumulative skid info.
    void SetWheelSkidInfoCumulative(int wheel, float skid);
    /// Set revolution per minute value for when wheel doesn't touch ground. If set to 0 (or not set), calculated from engine force (probably not what you want).
    void SetInAirRPM(float rpm);
    /// Set the coordinate system. The default is (0, 1, 2).
    void SetCoordinateSystem(const IntVector3& coordinateSystem = RIGHT_FORWARD_UP);
    /// Init the vehicle component after creation.
    void Init();
    /// Perform fixed step pre-update.
    void FixedUpdate(float timeStep) override;
    /// Perform fixed step post-update.
    void FixedPostUpdate(float timeStep) override;
    /// Perform variable step post-update.
    void PostUpdate(float timeStep) override;

    /// Return wheel position relative to RigidBody.
    Vector3 GetWheelPosition(int wheel);
    /// Return wheel rotation relative to RigidBody.
    Quaternion GetWheelRotation(int wheel);
    /// Return wheel connection point relative to RigidBody.
    Vector3 GetWheelConnectionPoint(int wheel) const;
    /// Return number of attached wheels.
    int GetNumWheels() const;
    /// Return node of the wheel.
    Node* GetWheelNode(int wheel) const;
    /// Return steering value of particular wheel.
    float GetSteeringValue(int wheel) const;
    /// Return suspension stiffness for particular wheel.
    float GetWheelSuspensionStiffness(int wheel) const;
    /// Return wheel max suspension force.
    float GetWheelMaxSuspensionForce(int wheel) const;
    /// Return wheel damping relaxation.
    float GetWheelDampingRelaxation(int wheel) const;
    /// Return wheel damping compression.
    float GetWheelDampingCompression(int wheel) const;
    /// Return wheel friction slip.
    float GetWheelFrictionSlip(int wheel) const;
    /// Return wheel roll influence.
    float GetWheelRollInfluence(int wheel) const;
    /// Return engine force for the wheel.
    float GetEngineForce(int wheel) const;
    /// Return hand brake value.
    float GetBrake(int wheel) const;
    /// Return wheel radius.
    float GetWheelRadius(int wheel) const;
    /// Return wheel rest length.
    void SetWheelRestLength(int wheel, float length);
    /// Return wheel rest length.
    float GetWheelRestLength(int wheel) const;
    /// Return maximum suspension travel value.
    float GetMaxSuspensionTravel(int wheel);
    /// Return wheel axle vector.
    Vector3 GetWheelAxle(int wheel) const;
    /// Return wheel slide speed.
    float GetWheelSideSlipSpeed(int wheel) const;
    /// Return side speed which is considered sliding.
    float GetMaxSideSlipSpeed() const;
    /// Sliding factor 0 <= x <= 1.
    float GetWheelSkidInfo(int wheel) const;
    /// Return wheel direction vector.
    Vector3 GetWheelDirection(int wheel) const;
    /// Return cumulative skid info.
    float GetWheelSkidInfoCumulative(int wheel) const;
    /// True if front wheel, otherwise false.
    bool IsFrontWheel(int wheel) const;
    /// Return wheel contact position.
    Vector3 GetContactPosition(int wheel) const;
    /// Return contact normal.
    Vector3 GetContactNormal(int wheel) const;
    /// Return revolution per minute value for when wheel doesn't touch ground.
    float GetInAirRPM() const;
    /// Return the coordinate system.
    IntVector3 GetCoordinateSystem() const { return coordinateSystem_; }

    /// Return wheel data attribute for serialization.
    VariantVector GetWheelDataAttr() const;
    /// Set wheel data attribute during loading.
    void SetWheelDataAttr(const VariantVector& value);

    /// Add debug geometry to the debug renderer.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// (0, 1, 2) coordinate system (default).
    static const IntVector3 RIGHT_UP_FORWARD;
    /// (0, 2, 1) coordinate system.
    static const IntVector3 RIGHT_FORWARD_UP;
    /// (1, 2, 0) coordinate system.
    static const IntVector3 UP_FORWARD_RIGHT;
    /// (1, 0, 2) coordinate system.
    static const IntVector3 UP_RIGHT_FORWARD;
    /// (2, 0, 1) coordinate system.
    static const IntVector3 FORWARD_RIGHT_UP;
    /// (2, 1, 0) coordinate system.
    static const IntVector3 FORWARD_UP_RIGHT;

private:
    /// If the RigidBody should be activated.
    bool activate_;
    /// Hull RigidBody.
    WeakPtr<RigidBody> hullBody_;
    /// Coordinate system.
    IntVector3 coordinateSystem_;
    /// Nodes of all wheels
    Vector<Node*> wheelNodes_;
    /// All wheels original rotations. These are applied in addition to wheel rotations by btRaycastVehicle.
    Vector<Quaternion> origRotation_;
    /// Revolutions per minute value for in-air motor wheels. TODO: Set per wheel.
    float inAirRPM_;
    /// Per-wheel extra settings.
    Vector<float> skidInfoCumulative_;
    /// Wheel side movement speed.
    Vector<float> wheelSideSlipSpeed_;
    /// Side slip speed threshold.
    float maxSideSlipSpeed_;
    /// Loaded data temporarily wait here for ApplyAttributes to come pick them up.
    VariantVector loadedWheelData_;

    struct RaycastVehicleData;
    /// Opaque Bullet data hidden from public.
    RaycastVehicleData* vehicleData_;
};

}
