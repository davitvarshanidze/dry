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
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "AnimatingScene.h"
#include "Rotator.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(AnimatingScene)

AnimatingScene::AnimatingScene(Context* context): Sample(context)
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    context->RegisterFactory<Rotator>();
}

void AnimatingScene::Start()
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

void AnimatingScene::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create the Octree component to the scene so that drawable objects can be rendered. Use default volume
    // (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->CreateComponent<Octree>();

    // Create a Zone component into a child scene node. The Zone controls ambient lighting and fog settings. Like the Octree,
    // it also defines its volume with a bounding box, but can be rotated (so it does not need to be aligned to the world X, Y
    // and Z axes.) Drawable objects "pick up" the zone they belong to and use it when rendering; several zones can exist
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>() };
    // Set same volume as the Octree, set a close bluish fog and some ambient light
    zone->SetBoundingBox(BoundingBox{ -1000.0f, 1000.0f });
    zone->SetAmbientColor({ 0.1f, 0.05f, 0.4f });
    zone->SetFogColor({ 0.125f, 0.0f, 0.15f });
    zone->SetFogStart(9.0f);
    zone->SetFogEnd(42.0f);

    // Create randomly positioned and oriented box StaticModels in the scene
    const unsigned NUM_OBJECTS{ 2000 };

    for (unsigned i{ 0 }; i < NUM_OBJECTS; ++i)
    {
        Node* boxNode{ scene_->CreateChild("Box") };
        boxNode->SetPosition(Vector3(Random(100.0f) - 50.0f, Random(100.0f) - 50.0f, Random(100.0f) - 50.0f));
        // Orient using random pitch, yaw and roll Euler angles
        boxNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
        StaticModel* boxObject{ boxNode->CreateComponent<StaticModel>() };
        boxObject->SetModel(cache->GetResource<Model>("Models/FancyBox.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/GrassRock.xml"));

        // Animate the box scale using a value animation.
        ValueAnimation* pulse{ new ValueAnimation(context_) };
        pulse->SetInterpolationMethod(IM_SINUSOIDAL);
        pulse->SetKeyFrame(0.0f,  Vector3::ONE * 0.5f);
        pulse->SetKeyFrame(0.25f, Vector3::ONE * 1.0f);
        pulse->SetKeyFrame(0.5f,  Vector3::ONE * 0.5f);
        pulse->SetKeyFrame(0.75f, Vector3::ONE * 1.5f);
        pulse->SetKeyFrame(1.0f,  Vector3::ONE * 0.5f);
        boxNode->SetAttributeAnimation("Scale", pulse);
        boxNode->SetAttributeAnimationTime("Scale", i * M_PHI);
        boxNode->SetAttributeAnimationSpeed("Scale", Random(0.5f, 1.0f));

        // Add our custom Rotator component which will rotate the scene node each frame, when the scene sends its update event.
        // The Rotator component derives from the base class LogicComponent, which has convenience functionality to subscribe
        // to the various update events, and forward them to virtual functions that can be implemented by subclasses. This way
        // writing logic/update components in C++ becomes similar to scripting.
        // Now we simply set same rotation speed for all objects
        Rotator* rotator{ boxNode->CreateComponent<Rotator>() };
        rotator->SetRotationSpeed(Vector3(10.0f, 20.0f, 30.0f));
    }

    // Create the camera. Let the starting position be at the world origin. As the fog limits maximum visible distance, we can
    // bring the far clip plane closer for more effective culling of distant objects
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(42.0f);

    // Create a point light to the camera scene node
    Light* light{ cameraNode_->CreateComponent<Light>() };
    light->SetLightType(LIGHT_POINT);
    light->SetRange(32.0f);
    light->SetBrightness(1.23f);
    light->SetColor({ 0.7f, 1.0f, 0.4f });
}

void AnimatingScene::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void AnimatingScene::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(AnimatingScene, HandleUpdate));
}

void AnimatingScene::MoveCamera(float timeStep)
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

void AnimatingScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}
