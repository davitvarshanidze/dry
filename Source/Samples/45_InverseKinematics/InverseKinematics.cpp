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

#include <Dry/Core/CoreEvents.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/AnimatedModel.h>
#include <Dry/Graphics/AnimationController.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/RibbonTrail.h>
#include <Dry/IK/IKEffector.h>
#include <Dry/IK/IKSolver.h>
#include <Dry/Input/Input.h>
#include <Dry/Math/Matrix2.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/Text3D.h>

#include "InverseKinematics.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(InverseKinematics)

InverseKinematics::InverseKinematics(Context* context) :
    Sample(context)
{
}

void InverseKinematics::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Left-Click and drag to look around\nRight-Click and drag to change incline\n"
                       "Press space to reset floor\nPress D to draw debug geometry");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);

    GetSubsystem<Input>()->SetMouseVisible(true);
}

void InverseKinematics::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    scene_->CreateComponent<PhysicsWorld>();

    // Create scene node & StaticModel component for showing a static plane
    floorNode_ = scene_->CreateChild("Plane");
    floorNode_->SetScale({ 50.0f, 1.0f, 50.0f });
    StaticModel* planeObject{ floorNode_->CreateComponent<StaticModel>() };
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Set up collision, we need to raycast to determine foot height
    floorNode_->CreateComponent<RigidBody>();
    CollisionShape* col{ floorNode_->CreateComponent<CollisionShape>() };
    col->SetBox(Vector3{ 1.0f, 0.0f, 1.0f });

    // Create a directional light to the world.
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection({ 0.6f, -1.0f, 0.8f }); // The direction vector does not need to be normalized
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters{ 0.00005f, 0.5f });
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters{ 10.0f, 50.0f, 200.0f, 0.0f, 0.8f });

    // Load Ozom model
    ozomNode_ = scene_->CreateChild("Ozom");
    ozomNode_->SetRotation(Quaternion{ 0.0f, 270.0f, 0.0f });
    AnimatedModel* ozom{ ozomNode_->CreateComponent<AnimatedModel>() };
    ozom->SetModel(cache->GetResource<Model>("Ghotiland/Models/Ozom.mdl"));
    ozom->SetMaterial(cache->GetResource<Material>("Ghotiland/Materials/Ozom.xml"));
    ozom->SetCastShadows(true);

    // Create animation controller and play walk animation
    ozomAnimCtrl_ = ozomNode_->CreateComponent<AnimationController>();
    ozomAnimCtrl_->PlayExclusive("Ghotiland/Anim/Ozom/Walk.ani", 0, true, 0.0f);

    // We need to attach two inverse kinematic effectors to Ozom's feet to
    // control the grounding.
    leftFoot_ = ozomNode_->GetChild("Instep.L", true);
    rightFoot_ = ozomNode_->GetChild("Instep.R", true);
    leftEffector_ = leftFoot_->CreateComponent<IKEffector>();
    rightEffector_ = rightFoot_->CreateComponent<IKEffector>();
    // Control 2 segments up to the hips
    leftEffector_->SetChainLength(2);
    rightEffector_->SetChainLength(2);

    // For the effectors to work, an IKSolver needs to be attached to one of
    // the parent nodes. Typically, you want to place the solver as close as
    // possible to the effectors for optimal performance. Since in this case
    // we're solving the legs only, we can place the solver at the spine.
    Node* spine{ ozomNode_->GetChild("Hips", true) };
    solver_ = spine->CreateComponent<IKSolver>();

    // Two-bone solver is more efficient and more stable than FABRIK (but only
    // works for two bones, obviously).
    solver_->SetAlgorithm(IKSolver::TWO_BONE);

    // Disable auto-solving, which means we need to call Solve() manually
    solver_->SetFeature(IKSolver::AUTO_SOLVE, false);

    // Only enable this so the debug draw shows us the pose before solving.
    // This should NOT be enabled for any other reason (it does nothing and is
    // a waste of performance).
    solver_->SetFeature(IKSolver::UPDATE_ORIGINAL_POSE, true);

    // Create the camera.
    cameraRotateNode_ = scene_->CreateChild("CameraRotate");
    cameraNode_ = cameraRotateNode_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition({ 0.0f, 0.0f, -4.0f });
    cameraRotateNode_->SetPosition({ 0.0f, 0.4f, 0.0f });
    pitch_ = 20;
    yaw_ = 50;
}

void InverseKinematics::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void InverseKinematics::UpdateCameraAndFloor(float /*timeStep*/)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input{ GetSubsystem<Input>() };

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY{ 0.1f };

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input->GetMouseButtonDown(MOUSEB_LEFT))
    {
        const IntVector2 mouseMove{ input->GetMouseMove() };
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);
    }

    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        const IntVector2 mouseMoveInt{ input->GetMouseMove() };
        const Vector2 mouseMove{ Matrix2{ -Cos(yaw_), Sin(yaw_),
                                           Sin(yaw_), Cos(yaw_) }
                                 * Vector2(mouseMoveInt.y_, -mouseMoveInt.x_) };

        floorPitch_ += MOUSE_SENSITIVITY * mouseMove.x_;
        floorPitch_ = Clamp(floorPitch_, -90.0f, 90.0f);
        floorRoll_ += MOUSE_SENSITIVITY * mouseMove.y_;
    }

    if (input->GetKeyPress(KEY_SPACE))
    {
        floorPitch_ = 0;
        floorRoll_ = 0;
    }

    if (input->GetKeyPress(KEY_D))
    {
        drawDebug_ = !drawDebug_;
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraRotateNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    floorNode_->SetRotation(Quaternion(floorPitch_, 0, floorRoll_));
}

void InverseKinematics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(InverseKinematics, HandleUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(InverseKinematics, HandlePostRenderUpdate));
    SubscribeToEvent(E_SCENEDRAWABLEUPDATEFINISHED, DRY_HANDLER(InverseKinematics, HandleSceneDrawableUpdateFinished));
}

void InverseKinematics::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    UpdateCameraAndFloor(timeStep);
}

void InverseKinematics::HandlePostRenderUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    if (drawDebug_)
        solver_->DrawDebugGeometry(false);
}

void InverseKinematics::HandleSceneDrawableUpdateFinished(StringHash /*eventType*/, VariantMap& eventData)
{
    PhysicsWorld* phyWorld{ scene_->GetComponent<PhysicsWorld>() };
    const Vector3 leftFootPosition{ leftFoot_->GetWorldPosition() };
    const Vector3 rightFootPosition{ rightFoot_->GetWorldPosition() };

    const float floorUpAngle{ floorNode_->GetWorldUp().Angle(Vector3::UP) };
    const float floorDownAngle{ floorNode_->GetWorldUp().Angle(Vector3::DOWN) };
    ozomNode_->SetPosition(Vector3::DOWN * .023f * Sqrt(Min(floorUpAngle, floorDownAngle)));

    // Cast ray down to get the normal of the underlying surface
    PhysicsRaycastResult result;
    phyWorld->RaycastSingle(result, Ray{ leftFootPosition + Vector3{ 0.0f, 1.0f, 0.0f },
                                         Vector3{ 0.0f, -1.0f, 0.0f } }, 2);
    if (result.body_)
    {
        // Cast again, but this time along the normal. Set the target position
        // to the ray intersection
        phyWorld->RaycastSingle(result, Ray{ leftFootPosition + result.normal_, -result.normal_ }, 2);
        // The foot node has an offset relative to the root node
        const float footOffset{ leftFoot_->GetWorldPosition().y_ - ozomNode_->GetWorldPosition().y_ };
        leftEffector_->SetTargetPosition(result.position_ + result.normal_ * footOffset);
        // Rotate foot according to normal
        leftFoot_->Rotate(Quaternion(Vector3(0, 1, 0), result.normal_), TS_WORLD);
    }

    // Same deal with the right foot
    phyWorld->RaycastSingle(result, Ray{ rightFootPosition + Vector3{ 0.0f, 1.0f, 0.0f },
                                         Vector3{ 0.0f, -1.0f, 0.0f } }, 2);
    if (result.body_)
    {
        phyWorld->RaycastSingle(result, Ray{ rightFootPosition + result.normal_, -result.normal_ }, 2);
        const float footOffset{ rightFoot_->GetWorldPosition().y_ - ozomNode_->GetWorldPosition().y_ };
        rightEffector_->SetTargetPosition(result.position_ + result.normal_ * footOffset);
        rightFoot_->Rotate(Quaternion{ Vector3{ 0.0f, 1.0f, 0.0f }, result.normal_ }, TS_WORLD);
    }

    solver_->Solve();
}
