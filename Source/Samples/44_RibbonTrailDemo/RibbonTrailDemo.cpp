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
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/RibbonTrail.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/Text3D.h>

#include "RibbonTrailDemo.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(RibbonTrailDemo)

RibbonTrailDemo::RibbonTrailDemo(Context* context): Sample(context),
    swordTrailStartTime_{ 0.2f },
    swordTrailEndTime_{ 0.46f },
    timeStepSum_{ 0.0f }
{
}

void RibbonTrailDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void RibbonTrailDemo::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->CreateComponent<Octree>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode{ scene_->CreateChild("Plane") };
    planeNode->SetScale({ 100.0f, 1.0f, 100.0f });
    StaticModel* planeObject{ planeNode->CreateComponent<StaticModel>() };
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a directional light to the world.
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection({ 0.6f, -1.0f, 0.8f }); // The direction vector does not need to be normalized
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters{ 0.00005f, 0.5f });
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters{ 10.0f, 50.0f, 200.0f, 0.0f, 0.8f });

    // Create first box for face camera trail demo with 1 column.
    boxNode1_ = scene_->CreateChild("Box1");
    StaticModel* box1{ boxNode1_->CreateComponent<StaticModel>() };
    box1->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    box1->SetCastShadows(true);
    RibbonTrail* boxTrail1{ boxNode1_->CreateComponent<RibbonTrail>() };
    boxTrail1->SetMaterial(cache->GetResource<Material>("Materials/RibbonTrail.xml"));
    boxTrail1->SetStartColor({ 1.0f, 0.5f, 0.0f, 1.0f });
    boxTrail1->SetEndColor({ 1.0f, 1.0f, 0.0f, 0.0f });
    boxTrail1->SetWidth(0.5f);
    boxTrail1->SetUpdateInvisible(true);

    // Create second box for face camera trail demo with 4 column.
    // This will produce less distortion than first trail.
    boxNode2_ = scene_->CreateChild("Box2");
    StaticModel* box2{ boxNode2_->CreateComponent<StaticModel>() };
    box2->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    box2->SetCastShadows(true);

    RibbonTrail* boxTrail2{ boxNode2_->CreateComponent<RibbonTrail>() };
    boxTrail2->SetMaterial(cache->GetResource<Material>("Materials/RibbonTrail.xml"));
    boxTrail2->SetStartColor({ 1.0f, 0.5f, 0.0f, 1.0f });
    boxTrail2->SetEndColor({ 1.0f, 1.0f, 0.0f, 0.0f });
    boxTrail2->SetWidth(0.5f);
    boxTrail2->SetTailColumn(4);
    boxTrail2->SetUpdateInvisible(true);

    // Load ninja animated model for bone trail demo.
    Node* ninjaNode{ scene_->CreateChild("Ninja") };
    ninjaNode->SetPosition({ 5.0f, 0.0f, 0.0f });
    ninjaNode->SetRotation(Quaternion{ 0.0f, 180.0f, 0.0f });
    AnimatedModel* ninja{ ninjaNode->CreateComponent<AnimatedModel>() };
    ninja->SetModel(cache->GetResource<Model>("Models/NinjaSnowWar/Ninja.mdl"));
    ninja->SetMaterial(cache->GetResource<Material>("Materials/NinjaSnowWar/Ninja.xml"));
    ninja->SetCastShadows(true);

    // Create animation controller and play attack animation.
    ninjaAnimCtrl_ = ninjaNode->CreateComponent<AnimationController>();
    ninjaAnimCtrl_->PlayExclusive("Models/NinjaSnowWar/Ninja_Attack3.ani", 0, true, 0.0f);

    // Add ribbon trail to tip of sword.
    Node* swordTip{ ninjaNode->GetChild("Joint29", true) };
    swordTrail_ = swordTip->CreateComponent<RibbonTrail>();

    // Set sword trail type to bone and set other parameters.
    swordTrail_->SetTrailType(TT_BONE);
    swordTrail_->SetMaterial(cache->GetResource<Material>("Materials/SlashTrail.xml"));
    swordTrail_->SetLifetime(0.22f);
    swordTrail_->SetStartColor({ 1.0f, 1.0f, 1.0f, 0.75f });
    swordTrail_->SetEndColor({ 0.2f, 0.5f, 1.0f, 0.0f });
    swordTrail_->SetTailColumn(4);
    swordTrail_->SetUpdateInvisible(true);

    // Add floating text for info.
    Node* boxTextNode1{ scene_->CreateChild("BoxText1") };
    boxTextNode1->SetPosition({ -1.0f, 2.0f, 0.0f });
    Text3D* boxText1{ boxTextNode1->CreateComponent<Text3D>() };
    boxText1->SetText("Face Camera Trail (4 Column)");
    boxText1->SetFont(cache->GetResource<Font>("Fonts/Days.ttf"), 24);

    Node* boxTextNode2{ scene_->CreateChild("BoxText2") };
    boxTextNode2->SetPosition({ -6.0f, 2.0f, 0.0f });
    Text3D* boxText2{ boxTextNode2->CreateComponent<Text3D>() };
    boxText2->SetText("Face Camera Trail (1 Column)");
    boxText2->SetFont(cache->GetResource<Font>("Fonts/Days.ttf"), 24);

    Node* ninjaTextNode2{ scene_->CreateChild("NinjaText") };
    ninjaTextNode2->SetPosition({ 4.0f, 2.5f, 0.0f });
    Text3D* ninjaText{ ninjaTextNode2->CreateComponent<Text3D>() };
    ninjaText->SetText("Bone Trail (4 Column)");
    ninjaText->SetFont(cache->GetResource<Font>("Fonts/Days.ttf"), 24);

    // Create the camera.
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition({ 0.0f, 2.0f, -14.0f });
}

void RibbonTrailDemo::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void RibbonTrailDemo::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input{ GetSubsystem<Input>() };

    // Movement speed as world units per second
    const float MOVE_SPEED{ 20.0f };
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY{ 0.1f };

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    const IntVector2 mouseMove{ input->GetMouseMove() };
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_E))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep, TS_WORLD);
    if (input->GetKeyDown(KEY_Q))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep, TS_WORLD);

}

void RibbonTrailDemo::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(RibbonTrailDemo, HandleUpdate));
}

void RibbonTrailDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    // Sum of timesteps.
    timeStepSum_ += timeStep;

    // Move first box with pattern.
    boxNode1_->SetTransform(Vector3{ -4.0f + 3.0f * Cos(100.0f * timeStepSum_), 0.5f, -2.0f * Cos(400.0f * timeStepSum_) },
        Quaternion());

    // Move second box with pattern.
    boxNode2_->SetTransform(Vector3{ 0.0f + 3.0f * Cos(100.0f * timeStepSum_), 0.5f, -2.0f * Cos(400.0f * timeStepSum_) },
        Quaternion());

    // Get elapsed attack animation time.
    const float swordAnimTime{ ninjaAnimCtrl_->GetAnimationState(String("Models/NinjaSnowWar/Ninja_Attack3.ani"))->GetTime() };

    // Stop emitting trail when sword is finished slashing.
    if (!swordTrail_->IsEmitting() && swordAnimTime > swordTrailStartTime_ && swordAnimTime < swordTrailEndTime_)
        swordTrail_->SetEmitting(true);
    else if (swordTrail_->IsEmitting() && swordAnimTime >= swordTrailEndTime_)
        swordTrail_->SetEmitting(false);
}
