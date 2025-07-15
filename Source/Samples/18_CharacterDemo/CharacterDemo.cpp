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
#include <Dry/Core/ProcessUtils.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/AnimatedModel.h>
#include <Dry/Graphics/AnimationController.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/Light.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/Skybox.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Controls.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "Character.h"
#include "CharacterDemo.h"
#include "Touch.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(CharacterDemo)

CharacterDemo::CharacterDemo(Context* context): Sample(context),
    firstPerson_{ false }
{
    // Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
    Character::RegisterObject(context);
}

CharacterDemo::~CharacterDemo() = default;

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    if (touchEnabled_)
        touch_ = new Touch{ context_, TOUCH_SENSITIVITY };

    // Create static scene content
    CreateScene();

    // Create the controllable character
    CreateCharacter();

    // Create the UI content
    CreateInstructions("Use WASD keys and mouse/touch to move\n"
                       "Space to jump, F to toggle 1st/3rd person\n"
                       "F5 to save scene, F7 to load");

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node{ context_ };
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetNearClip(0.45f);
    camera->SetFarClip(160.0f);

    GetSubsystem<Renderer>()->SetViewport(0, new Viewport{ context_, scene_, camera });

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>()};
    zone->SetAmbientColor({ 0.2f, 0.23f, 0.27f });
    zone->SetFogColor({ 0.9f, 0.93f, 0.95f });
    zone->SetFogStart(40.0f);
    zone->SetFogEnd(160.0f);
    zone->SetBoundingBox({ -1000.0f, 1000.0f });

    // Create a directional light with cascaded shadow mapping
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection({ -0.8f, -1.0f, 0.7f });
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetBrightness(1.1f);
    light->SetCastShadows(true);
    light->SetShadowIntensity(0.3f);
    light->SetShadowBias({ 0.00025f, 0.5f });
    light->SetShadowCascade({ 10.0f, 50.0f, 200.0f, 0.0f, 0.8f });

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    Node* skyNode{ scene_->CreateChild("Sky") };
    skyNode->SetScale(500.0f); // The scale actually does not matter
    Skybox* skybox{ skyNode->CreateComponent<Skybox>() };
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create the floor object
    Node* floorNode{ scene_->CreateChild("Floor") };
    floorNode->SetPosition({ 0.0f, -0.5f, 0.0f });
    floorNode->SetScale({ 400.0f, 1.0f, 400.0f });
    StaticModel* object{ floorNode->CreateComponent<StaticModel>() };
    object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Materials/MudLeavesTiled.xml"));

    RigidBody* body{ floorNode->CreateComponent<RigidBody>() };
    // Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
    // inside geometry
    body->SetCollisionLayer(2);
    CollisionShape* shape{ floorNode->CreateComponent<CollisionShape>() };
    shape->SetBox(Vector3::ONE);

    // Create mushrooms of varying sizes
    const unsigned NUM_MUSHROOMS{ 60 };

    for (unsigned i{ 0 }; i < NUM_MUSHROOMS; ++i)
    {
        Node* objectNode{ scene_->CreateChild("Mushroom") };
        objectNode->SetPosition({ Random(180.0f) - 90.0f, 0.0f, Random(180.0f) - 90.0f });
        objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        objectNode->SetScale(Max(0.1f, RandomNormal(6.0f, 3.0f)));

        StaticModel* object{ objectNode->CreateComponent<StaticModel>() };
        object->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        object->SetCastShadows(true);

        RigidBody* body{ objectNode->CreateComponent<RigidBody>() };
        body->SetCollisionLayer(2);
        CollisionShape* shape{ objectNode->CreateComponent<CollisionShape>() };
        shape->SetTriangleMesh(object->GetModel(), 0);
    }

    // Create movable boxes. Let them fall from the sky at first
    const unsigned NUM_BOXES{ 100 };

    for (unsigned i{ 0 }; i < NUM_BOXES; ++i)
    {
        const float scale{ Max(0.1f, RandomNormal(2.3f, 0.7f)) };

        Node* objectNode{ scene_->CreateChild("Box") };
        objectNode->SetPosition(Vector3(RandomNormal(0.0f, 100.0f), Random(20.0f) + 10.0f, Random(60.0f) + 10.0f));
        objectNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
        objectNode->SetScale(scale);

        StaticModel* object{ objectNode->CreateComponent<StaticModel>() };
        object->SetModel(cache->GetResource<Model>("Models/FancyBox.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/GrassRock.xml"));
        object->SetCastShadows(true);

        RigidBody* body{ objectNode->CreateComponent<RigidBody>() };
        body->SetCollisionLayer(2);
        body->SetFriction(0.7f);
        // Bigger boxes will be heavier and harder to move
        body->SetMass(scale * scale * scale * 1.5f);
        CollisionShape* shape{ objectNode->CreateComponent<CollisionShape>() };
        shape->SetBox(Vector3::ONE);
        shape->SetMargin(0.01f);
    }
}

void CharacterDemo::CreateCharacter()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    Node* objectNode{ scene_->CreateChild("Ozom") };
    objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    // Create the rendering component + animation controller
    AnimatedModel* object{ objectNode->CreateComponent<AnimatedModel>() };
    object->SetModel(cache->GetResource<Model>("Ghotiland/Models/Ozom.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Ghotiland/Materials/Ozom.xml"));
    object->SetCastShadows(true);
    objectNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    object->GetSkeleton().GetBone("Head")->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    RigidBody* body{ objectNode->CreateComponent<RigidBody>() };
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    CollisionShape* shape{ objectNode->CreateComponent<CollisionShape>() };
    shape->SetCapsule(0.7f, 1.8f, Vector3{ 0.0f, 0.9f, 0.0f });

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    character_ = objectNode->CreateComponent<Character>();
}

void CharacterDemo::SubscribeToEvents()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(CharacterDemo, HandleUpdate));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    SubscribeToEvent(E_POSTUPDATE, DRY_HANDLER(CharacterDemo, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Input* input{ GetSubsystem<Input>() };

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using touch utility class
        if (touch_)
            touch_->UpdateTouches(character_->controls_);

        // Update controls using keys
        UI* ui{ GetSubsystem<UI>() };

        if (!ui->GetFocusElement())
        {
            if (!touch_ || !touch_->useGyroscope_)
            {
                character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
                character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
                character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
                character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            }
            character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));

            // Add character yaw & pitch from the mouse motion or touch input
            if (touchEnabled_)
            {
                for (unsigned i{ 0 }; i < input->GetNumTouches(); ++i)
                {
                    TouchState* state{ input->GetTouch(i) };

                    if (!state->touchedElement_)    // Touch on empty space
                    {
                        Camera* camera{ cameraNode_->GetComponent<Camera>() };

                        if (!camera)
                            return;

                        Graphics* graphics{ GetSubsystem<Graphics>() };
                        character_->controls_.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                        character_->controls_.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
                    }
                }
            }
            else
            {
                character_->controls_.yaw_ += input->GetMouseMoveX() * YAW_SENSITIVITY;
                character_->controls_.pitch_ += input->GetMouseMoveY() * YAW_SENSITIVITY;
            }
            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
            // Set rotation already here so that it's updated every rendering frame instead of every physics frame
            character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));

            // Switch between 1st and 3rd person
            if (input->GetKeyPress(KEY_F))
                firstPerson_ = !firstPerson_;

            // Turn on/off gyroscope on mobile platform
            if (touch_ && input->GetKeyPress(KEY_G))
                touch_->useGyroscope_ = !touch_->useGyroscope_;

            // Check for loading / saving the scene
            if (input->GetKeyPress(KEY_F5))
            {
                File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_WRITE);
                scene_->SaveXML(saveFile);
            }
            if (input->GetKeyPress(KEY_F7))
            {
                File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_READ);
                scene_->LoadXML(loadFile);
                // After loading we have to reacquire the weak pointer to the Character component, as it has been recreated
                // Simply find the character's scene node by name as there's only one of them
                Node* characterNode{ scene_->GetChild("Ozom", true) };

                if (characterNode)
                    character_ = characterNode->GetComponent<Character>();
            }
        }
    }
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode{ character_->GetNode() };

    // Get camera lookat dir from character yaw + pitch
    const Quaternion& rot{ characterNode->GetRotation() };
    Quaternion dir{ rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT) };

    // Turn head to camera pitch, but limit to avoid unnatural animation
    Node* headNode{ characterNode->GetChild("Head", true) };
    const float limitPitch{ Clamp(character_->controls_.pitch_, -45.0f, 45.0f) };
    const Quaternion headDir{ rot * Quaternion{ limitPitch, Vector3(1.0f, 0.0f, 0.0f) } };
    // This could be expanded to look at an arbitrary target, now just look at a point in front
    const Vector3 headWorldTarget{ headNode->GetWorldPosition() + headDir * Vector3::FORWARD };
    headNode->LookAt(headWorldTarget, Vector3{ 0.0f, 1.0f, 0.0f });

    if (firstPerson_)
    {
        cameraNode_->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.2f));
        cameraNode_->SetRotation(dir);
    }
    else
    {
        // Third person camera: position behind the character
        Vector3 aimPoint{ characterNode->GetPosition() + rot * Vector3{ 0.0f, 1.7f, 0.0f } };

        // Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
        const Vector3 rayDir{ dir * Vector3::BACK };
        float rayDistance{ (touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST) };
        PhysicsRaycastResult result;
        scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);

        if (result.body_)
            rayDistance = Min(rayDistance, result.distance_);

        rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

        cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
        cameraNode_->SetRotation(dir);
    }
}
