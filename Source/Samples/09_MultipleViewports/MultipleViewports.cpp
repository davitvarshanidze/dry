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
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Light.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/RenderPath.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "MultipleViewports.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(MultipleViewports)

MultipleViewports::MultipleViewports(Context* context): Sample(context),
    drawDebug_(false)
{
}

void MultipleViewports::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move\n"
                       "B to toggle bloom, F to toggle FXAA\n"
                       "Space to toggle debug geometry\n");

    // Setup the viewports for displaying the scene
    SetupViewports();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void MultipleViewports::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode{ scene_->CreateChild("Plane") };
    planeNode->SetScale({ 100.0f, 1.0f, 100.0f });
    StaticModel* planeObject{ planeNode->CreateComponent<StaticModel>() };
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>() };
    zone->SetBoundingBox(BoundingBox{ -1000.0f, 1000.0f });
    zone->SetAmbientColor({ 0.15f, 0.15f, 0.15f });
    zone->SetFogColor({ 0.5f, 0.5f, 0.7f });
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters{ 0.00025f, 0.5f });
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters{ 10.0f, 50.0f, 200.0f, 0.0f, 0.8f });

    // Create some mushrooms
    const unsigned NUM_MUSHROOMS{ 240 };

    for (unsigned i{ 0 }; i < NUM_MUSHROOMS; ++i)
    {
        Node* mushroomNode{ scene_->CreateChild("Mushroom") };
        mushroomNode->SetPosition(Vector3{ Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f });
        mushroomNode->SetRotation(Quaternion{ 0.0f, Random(360.0f), 0.0f });
        mushroomNode->SetScale(0.5f + Random(2.0f));

        StaticModel* mushroomObject{ mushroomNode->CreateComponent<StaticModel>() };
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        mushroomObject->SetCastShadows(true);
    }

    // Create randomly sized boxes. If boxes are big enough, make them occluders
    const unsigned NUM_BOXES{ 20 };

    for (unsigned i{ 0 }; i < NUM_BOXES; ++i)
    {
        Node* boxNode{ scene_->CreateChild("Box") };
        const float size{ 1.0f + Random(10.0f) };

        boxNode->SetPosition(Vector3{ Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f });
        boxNode->SetScale(size);

        StaticModel* boxObject{ boxNode->CreateComponent<StaticModel>() };
        boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        boxObject->SetCastShadows(true);

        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create the cameras. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(300.0f);

    // Parent the rear camera node to the front camera node and turn it 180 degrees to face backward
    // Here, we use the angle-axis constructor for Quaternion instead of the usual Euler angles
    rearCameraNode_ = cameraNode_->CreateChild("RearCamera");
    rearCameraNode_->Rotate(Quaternion(180.0f, Vector3::UP));
    Camera* rearCamera{ rearCameraNode_->CreateComponent<Camera>() };
    rearCamera->SetFarClip(300.0f);
    // Because the rear viewport is rather small, disable occlusion culling from it. Use the camera's
    // "view override flags" for this. We could also disable eg. shadows or force low material quality
    // if we wanted
    rearCamera->SetViewOverrideFlags(VO_DISABLE_OCCLUSION);

    // Set an initial position for the front camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void MultipleViewports::SetupViewports()
{
    Graphics* graphics{ GetSubsystem<Graphics>() };
    Renderer* renderer{ GetSubsystem<Renderer>() };

    renderer->SetNumViewports(2);

    // Set up the front camera viewport
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);

    // Clone the default render path so that we do not interfere with the other viewport, then add
    // bloom and FXAA post process effects to the front viewport. Render path commands can be tagged
    // for example with the effect name to allow easy toggling on and off. We start with the effects
    // disabled.
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    SharedPtr<RenderPath> effectRenderPath{ viewport->GetRenderPath()->Clone() };
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Bloom.xml"));
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA2.xml"));
    // Make the bloom mixing parameter more pronounced
    effectRenderPath->SetShaderParameter("BloomMix", Vector2{ 0.95f, 0.9f });
    effectRenderPath->SetShaderParameter("BloomThreshold", 0.5f);
    effectRenderPath->SetEnabled("Bloom", false);
    effectRenderPath->SetEnabled("FXAA2", false);
    viewport->SetRenderPath(effectRenderPath);

    // Set up the rear camera viewport on top of the front view ("rear view mirror")
    // The viewport index must be greater in that case, otherwise the view would be left behind
    SharedPtr<Viewport> rearViewport{ new Viewport{ context_, scene_, rearCameraNode_->GetComponent<Camera>(),
        IntRect(graphics->GetWidth() * 2 / 3, 32, graphics->GetWidth() - 32, graphics->GetHeight() / 3) } };
    renderer->SetViewport(1, rearViewport);
}

void MultipleViewports::SubscribeToEvents()
{
    // Subscribe HandleUpdate() method for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(MultipleViewports, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() method for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(MultipleViewports, HandlePostRenderUpdate));
}

void MultipleViewports::MoveCamera(float timeStep)
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

    // Toggle post processing effects on the front viewport. Note that the rear viewport is unaffected
    RenderPath* effectRenderPath{ GetSubsystem<Renderer>()->GetViewport(0)->GetRenderPath() };

    if (input->GetKeyPress(KEY_B))
        effectRenderPath->ToggleEnabled("Bloom");
    if (input->GetKeyPress(KEY_F))
        effectRenderPath->ToggleEnabled("FXAA2");

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

void MultipleViewports::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void MultipleViewports::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry, which will show eg. drawable bounding boxes and skeleton
    // bones. Disable depth test so that we can see the effect of occlusion
    if (drawDebug_)
        GetSubsystem<Renderer>()->DrawDebugGeometry(false);
}
