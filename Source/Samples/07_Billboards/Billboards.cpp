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
#include <Dry/Graphics/BillboardSet.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Light.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "Billboards.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(Billboards)

Billboards::Billboards(Context* context) :
    Sample(context),
    drawDebug_(false)
{
}

void Billboards::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move\n"
                       "Space to toggle debug geometry");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void Billboards::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>() };
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.2f, 0.2f, 0.2f));
    light->SetSpecularIntensity(1.0f);

    // Create a "floor" consisting of several tiles
    for (int y{ -5 }; y <= 5; ++y)
    {
        for (int x{ -5 }; x <= 5; ++x)
        {
            Node* floorNode{ scene_->CreateChild("FloorTile") };
            floorNode->SetPosition({ x * 20.5f, -0.5f, y * 20.5f });
            floorNode->SetScale({ 20.0f, 1.0f, 20.f });

            StaticModel* floorObject{ floorNode->CreateComponent<StaticModel>() };
            floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        }
    }

    // Create groups of mushrooms, which act as shadow casters
    const unsigned NUM_MUSHROOMGROUPS{ 25 };
    const unsigned NUM_MUSHROOMS{ 25 };

    for (unsigned i{ 0 }; i < NUM_MUSHROOMGROUPS; ++i)
    {
        // First create a scene node for the group. The individual mushrooms nodes will be created as children
        Node* groupNode{ scene_->CreateChild("MushroomGroup") };
        groupNode->SetPosition(Vector3(Random(190.0f) - 95.0f, 0.0f, Random(190.0f) - 95.0f));

        for (unsigned j{ 0 }; j < NUM_MUSHROOMS; ++j)
        {
            Node* mushroomNode{ groupNode->CreateChild("Mushroom") };
            mushroomNode->SetPosition(Vector3(Random(25.0f) - 12.5f, 0.0f, Random(25.0f) - 12.5f));
            mushroomNode->SetRotation(Quaternion(0.0f, Random() * 360.0f, 0.0f));
            mushroomNode->SetScale(1.0f + Random() * 4.0f);

            StaticModel* mushroomObject{ mushroomNode->CreateComponent<StaticModel>() };
            mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
            mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
            mushroomObject->SetCastShadows(true);
        }
    }

    // Create billboard sets (floating smoke)
    const unsigned NUM_BILLBOARDNODES = 25;
    const unsigned NUM_BILLBOARDS = 10;

    for (unsigned i{ 0 }; i < NUM_BILLBOARDNODES; ++i)
    {
        Node* smokeNode{ scene_->CreateChild("Smoke") };
        smokeNode->SetPosition({ Random(200.0f) - 100.0f, Random(20.0f) + 10.0f, Random(200.0f) - 100.0f });

        BillboardSet* billboardObject{ smokeNode->CreateComponent<BillboardSet>() };
        billboardObject->SetNumBillboards(NUM_BILLBOARDS);
        billboardObject->SetMaterial(cache->GetResource<Material>("Materials/LitSmoke.xml"));
        billboardObject->SetSorted(true);

        for (unsigned j{ 0 }; j < NUM_BILLBOARDS; ++j)
        {
            Billboard* bb{ billboardObject->GetBillboard(j) };
            bb->position_ = Vector3{ Random(12.0f) - 6.0f, Random(8.0f) - 4.0f, Random(12.0f) - 6.0f };
            bb->size_ = Vector2{ Random(2.0f) + 3.0f, Random(2.0f) + 3.0f };
            bb->rotation_ = Random() * 360.0f;
            bb->enabled_ = true;
        }

        // After modifying the billboards, they need to be "committed" so that the BillboardSet updates its internals
        billboardObject->Commit();
    }

    // Create shadow casting spotlights
    const unsigned NUM_LIGHTS{ 9 };

    for (unsigned i{ 0 }; i < NUM_LIGHTS; ++i)
    {
        Node* lightNode{ scene_->CreateChild("SpotLight") };
        Light* light{ lightNode->CreateComponent<Light>() };

        const Vector3 position{ (i % 3) * 60.0f - 60.0f, 45.0f, (i / 3.f) * 60.0f - 60.0f };
        const Color color{ ((i + 1) & 1u) * 0.5f + 0.5f, (((i + 1) >> 1u) & 1u) * 0.5f + 0.5f, (((i + 1) >> 2u) & 1u) * 0.5f + 0.5f };
        float angle{ 0.0f };

        lightNode->SetPosition(position);
        lightNode->SetDirection({ Sin(angle), -1.5f, Cos(angle) });

        light->SetLightType(LIGHT_SPOT);
        light->SetRange(90.0f);
        light->SetRampTexture(cache->GetResource<Texture2D>("Textures/RampExtreme.png"));
        light->SetFov(45.0f);
        light->SetColor(color);
        light->SetSpecularIntensity(1.0f);
        light->SetCastShadows(true);
        light->SetShadowBias({ 0.00002f, 0.0f });

        // Configure shadow fading for the lights. When they are far away enough, the lights eventually become unshadowed for
        // better GPU performance. Note that we could also set the maximum distance for each object to cast shadows
        light->SetShadowFadeDistance(100.0f); // Fade start distance
        light->SetShadowDistance(125.0f); // Fade end distance, shadows are disabled
        // Set half resolution for the shadow maps for increased performance
        light->SetShadowResolution(0.5f);
        // The spot lights will not have anything near them, so move the near plane of the shadow camera farther
        // for better shadow depth resolution
        light->SetShadowNearFarRatio(0.01f);
    }

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3{ 0.0f, 5.0f, -23.0f });
}

void Billboards::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void Billboards::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(Billboards, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(Billboards, HandlePostRenderUpdate));
}

void Billboards::MoveCamera(float timeStep)
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

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

void Billboards::AnimateScene(float timeStep)
{
    // Get the light and billboard scene nodes
    PODVector<Node*> lightNodes;
    PODVector<Node*> billboardNodes;
    scene_->GetChildrenWithComponent<Light>(lightNodes);
    scene_->GetChildrenWithComponent<BillboardSet>(billboardNodes);

    const float LIGHT_ROTATION_SPEED{ 20.0f };
    const float BILLBOARD_ROTATION_SPEED{ 50.0f };

    // Rotate the lights around the world Y-axis
    for (unsigned i{ 0 }; i < lightNodes.Size(); ++i)
        lightNodes[i]->Rotate(Quaternion{ 0.0f, LIGHT_ROTATION_SPEED * timeStep, 0.0f }, TS_WORLD);

    // Rotate the individual billboards within the billboard sets, then recommit to make the changes visible
    for (unsigned i{ 0 }; i < billboardNodes.Size(); ++i)
    {
        BillboardSet* billboardObject{ billboardNodes[i]->GetComponent<BillboardSet>() };

        for (unsigned j{ 0 }; j < billboardObject->GetNumBillboards(); ++j)
        {
            Billboard* bb{ billboardObject->GetBillboard(j) };
            bb->rotation_ += BILLBOARD_ROTATION_SPEED * timeStep;
        }

        billboardObject->Commit();
    }
}

void Billboards::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera and animate the scene, scale movement with time step
    MoveCamera(timeStep);
    AnimateScene(timeStep);
}

void Billboards::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry. This time use depth test, as otherwise the result becomes
    // hard to interpret due to large object count
    if (drawDebug_)
        GetSubsystem<Renderer>()->DrawDebugGeometry(true);
}
