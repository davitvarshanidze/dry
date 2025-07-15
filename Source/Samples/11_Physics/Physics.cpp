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
#include <Dry/Graphics/Skybox.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/File.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "Physics.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(Physics)

Physics::Physics(Context* context) :
    Sample(context),
    drawDebug_(false)
{
}

void Physics::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move\n"
                       "LMB to spawn physics objects\n"
                       "F5 to save scene, F7 to load\n"
                       "Space to toggle physics debug geometry");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void Physics::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene{ context_ };

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Create a physics simulation world with default parameters, which will update at 60fps. Like the Octree must
    // exist before creating drawable components, the PhysicsWorld must exist before creating physics components.
    // Finally, create a DebugRenderer component so that we can draw physics debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    PhysicsWorld* physicsWorld{ scene_->CreateComponent<PhysicsWorld>() };
    physicsWorld->SetGravity(Vector3::DOWN * 17.0f);
    physicsWorld->SetFps(70.0f);
    physicsWorld->SetSplitImpulse(true);

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>() };
    zone->SetBoundingBox({ -1000.0f, 1000.0f });
    zone->SetAmbientColor({ 0.15f, 0.15f, 0.15f });
    zone->SetFogColor({ 0.9f, 0.93f, 0.95f });
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection({ -0.8f, -1.0f, 0.7f });
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetBrightness(1.1f);
    light->SetCastShadows(true);
    light->SetShadowIntensity(0.3f);
    light->SetShadowBias({ 0.00025f, 0.5f });
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade({ 10.0f, 50.0f, 200.0f, 0.0f, 0.8f });

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    Node* skyNode{ scene_->CreateChild("Sky") };
    skyNode->SetScale(500.0f); // The scale actually does not matter
    Skybox* skybox{ skyNode->CreateComponent<Skybox>() };
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    {
        // Create a floor object, 1000 x 1000 world units. Adjust position so that the ground is at zero Y
        Node* floorNode{ scene_->CreateChild("Floor") };
        floorNode->SetPosition({ 0.0f, -0.5f, 0.0f });
        floorNode->SetScale({ 1000.0f, 1.0f, 1000.0f });
        StaticModel* floorObject{ floorNode->CreateComponent<StaticModel>() };
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

        // Make the floor physical by adding RigidBody and CollisionShape components. The RigidBody's default
        // parameters make the object static (zero mass.) Note that a CollisionShape by itself will not participate
        // in the physics simulation
        /*RigidBody* body = */floorNode->CreateComponent<RigidBody>();
        CollisionShape* shape{ floorNode->CreateComponent<CollisionShape>() };
        // Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene node scale, so the
        // rendering and physics representation sizes should match (the box model is also 1 x 1 x 1.)
        shape->SetBox(Vector3::ONE);
    }

    {
        // Create a pyramid of movable physics objects
        const int n{ 9 };

        for (int y{ 0 }; y < n; ++y)
        for (int x{ 0 }; x < n - y; ++x)
        {
            Node* canNode{ scene_->CreateChild("Can") };
            canNode->SetPosition({ x + 0.5f * (y % 2 - n) + y / 2 + 0.5f, y + 0.5f, 0.0f });

            StaticModel* canObject{ canNode->CreateComponent<StaticModel>() };
            canObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
            canObject->SetMaterial(cache->GetResource<Material>("Materials/Chrome.xml"));
            canObject->SetCastShadows(true);

            // Create RigidBody and CollisionShape components like above. Give the RigidBody mass to make it movable
            // and also adjust friction. The actual mass is not important; only the mass ratios between colliding
            // objects are significant
            RigidBody* body{ canNode->CreateComponent<RigidBody>() };
            body->SetMass(0.75f);
            body->SetFriction(0.5f);
            body->SetRollingFriction(0.05f);
            body->SetLinearDamping(0.05f);
            body->SetAngularDamping(0.05f);
            body->SetLinearRestThreshold(0.2f);
            body->SetAngularRestThreshold(0.3f);
            body->SetRestitution(0.25f);
            CollisionShape* shape{ canNode->CreateComponent<CollisionShape>() };
            shape->SetCylinder(1.0f, 1.0f);
        }
    }

    // Create the camera. Set far clip to match the fog. Note: now we actually create the camera node outside the scene, because
    // we want it to be unaffected by scene load / save
    cameraNode_ = new Node{ context_ };
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(500.0f);

    // Set an initial position for the camera scene node above the floor
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, -20.0f));
}

void Physics::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() });
    renderer->SetViewport(0, viewport);
}

void Physics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(Physics, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(Physics, HandlePostRenderUpdate));
}

void Physics::MoveCamera(float timeStep)
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

    // "Shoot" a physics object with left mousebutton
    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        SpawnObject();

    // Check for loading/saving the scene. Save the scene to the file Data/Scenes/Physics.xml relative to the executable
    // directory
    if (input->GetKeyPress(KEY_F5))
    {
        File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Physics.xml", FILE_WRITE);
        scene_->SaveXML(saveFile);
    }
    if (input->GetKeyPress(KEY_F7))
    {
        File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Physics.xml", FILE_READ);
        scene_->LoadXML(loadFile);
    }

    // Toggle physics debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

void Physics::SpawnObject()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Create a ball at camera position
    Node* ballNode = scene_->CreateChild("Ball");
    ballNode->SetPosition(cameraNode_->GetPosition() + Vector3::DOWN * 0.5f);
    ballNode->SetScale(0.5f);
    StaticModel* ballObject{ ballNode->CreateComponent<StaticModel>() };
    ballObject->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
    ballObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
    ballObject->SetCastShadows(true);

    // Create physics components, use a smaller mass also
    RigidBody* body{ ballNode->CreateComponent<RigidBody>() };
    body->SetMass(3.0f);
    body->SetFriction(0.25f);
    CollisionShape* shape{ ballNode->CreateComponent<CollisionShape>() };
    shape->SetSphere(1.0f);

    const float ballVelocity{ 42.0f };

    // Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
    // to overcome gravity better
    body->ApplyImpulse(cameraNode_->GetRotation() * Vector3{ 0.0f, 0.075f, 1.0f }.Normalized() * body->GetMass() * ballVelocity);
    body->ApplyTorqueImpulse(cameraNode_->GetDirection() * body->GetMass() * Random(-0.5f, 0.5f));
}

void Physics::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat()} ;

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void Physics::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
    if (drawDebug_)
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}
