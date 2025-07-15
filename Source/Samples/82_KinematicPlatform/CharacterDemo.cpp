//
// Copyright (c) 2008-2019 the Urho3D project.
// Copyright (c) 2022-2022 LucKey Productions.
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
#include <Dry/Graphics/Zone.h>
#include <Dry/Graphics/Skybox.h>
#include <Dry/Input/Controls.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/KinematicCharacterController.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/IO/MemoryBuffer.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/Engine/DebugHud.h>

#include "Character.h"
#include "CharacterDemo.h"

#include "Touch.h"
#include "Lift.h"
#include "MovingPlatform.h"
#include "SplinePlatform.h"
#include "CollisionLayer.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(CharacterDemo)


CharacterDemo::CharacterDemo(Context* context): Sample(context),
    firstPerson_{ false },
    drawDebug_{ false }
{
    Character::RegisterObject(context);
    Lift::RegisterObject(context);
    MovingPlatform::RegisterObject(context);
    SplinePlatform::RegisterObject(context);
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Setup()
{
    engineParameters_["WindowTitle"]  = GetTypeName();
    engineParameters_["LogName"]      = GetSubsystem<FileSystem>()->GetProgramDir() + "kinematicplatform.log";
    engineParameters_["FullScreen"]   = false;
    engineParameters_["Headless"]     = false;
    engineParameters_["WindowWidth"]  = 1280;
    engineParameters_["WindowHeight"] = 768;
}

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);

    ChangeDebugHudText();

    // Create static scene content
    CreateScene();

    // Create the controllable character
    CreateCharacter();

    // Create the UI content
    CreateInstructions();

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::ChangeDebugHudText()
{
    // change profiler text
    if (GetSubsystem<DebugHud>())
    {
        Text* dbgText{ GetSubsystem<DebugHud>()->GetProfilerText() };
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetStatsText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetMemoryText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetModeText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);
    }
}

void CharacterDemo::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Renderer* renderer{ GetSubsystem<Renderer>() };

    scene_ = context_->CreateObject<Scene>();

    cameraNode_ = context_->CreateObject<Node>();
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(200.0f);
    renderer->SetViewport(0, new Viewport{ context_, scene_, camera });

    // Create a mathematical plane to represent the water in calculations
    Plane waterPlane{ Vector3::UP, Vector3::UP * 6.95 };
    // Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive clipping
    Plane waterClipPlane{ Vector3::UP, Vector3::UP * 6.95 + Vector3::DOWN * 0.025f };

    // Create camera for water reflection
    // It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
    // its position when rendering
    Node* reflectionCameraNode{ cameraNode_->CreateChild("Reflection") };
    Camera* reflectionCamera{ reflectionCameraNode->CreateComponent<Camera>() };
    reflectionCamera->SetFarClip(750.0f);
    reflectionCamera->SetViewMask(0xffffff7f);
    reflectionCamera->SetAutoAspectRatio(false);
    reflectionCamera->SetUseReflection(true);
    reflectionCamera->SetReflectionPlane(waterPlane);
    reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
    reflectionCamera->SetClipPlane(waterClipPlane);
    // The water reflection texture is rectangular. Set reflection camera aspect ratio to match
    reflectionCamera->SetAspectRatio(camera->GetAspectRatio());
    // View override flags could be used to optimize reflection rendering. For example disable shadows
    //reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);

    // Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
    // texture unit of the water material

    const int texSize{ 2048 };
    SharedPtr<Texture2D> renderTexture{ new Texture2D{ context_ } };
    renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    renderTexture->SetFilterMode(FILTER_BILINEAR);
    RenderSurface* surface{ renderTexture->GetRenderSurface() };
    SharedPtr<Viewport> rttViewport{ new Viewport{ context_, scene_, reflectionCamera } };
    surface->SetViewport(0, rttViewport);
    Material* waterMat{ cache->GetResource<Material>("Ghotiland/Materials/Water.xml") };
    waterMat->SetTexture(TU_DIFFUSE, renderTexture);

    // load scene
    XMLFile* xmlLevel{ cache->GetResource<XMLFile>("Ghotiland/Home.xml") };
    scene_->LoadXML(xmlLevel->GetRoot());
    scene_->GetComponent<PhysicsWorld>()->SetFps(70);

    // Sync distant fog color
    Material* distantFog{ scene_->GetChild("Sky")->GetComponent<Skybox>()->GetMaterial(1) };
    if (distantFog)
    {
        Zone* zone{ scene_->GetComponent<Zone>() };
        distantFog->SetShaderParameter("MatDiffColor", zone->GetFogColor());
    }

    // init platforms
    Lift* lift{ scene_->CreateComponent<Lift>() };
    Node* liftNode{ scene_->GetChild("Lift", true) };
    lift->Initialize(liftNode, liftNode->GetWorldPosition() + Vector3::UP * 10.f);

    MovingPlatform* movingPlatform{ scene_->CreateComponent<MovingPlatform>() };
    Node* movingPlatNode{ scene_->GetChild("LilyPlatform1", true) };
    movingPlatform->Initialize(movingPlatNode, movingPlatNode->GetWorldPosition() + Vector3::FORWARD * 17.0f, true);

    SplinePlatform* splinePlatform{ scene_->CreateComponent<SplinePlatform>() };
    Node* splineNode{ scene_->GetChild("splinePath1", true) };
    splinePlatform->Initialize(splineNode);


    Node* ozomcopterNode{ scene_->CreateChild("Ozomcopter") };
    StaticModel* copterModel{ ozomcopterNode->CreateComponent<StaticModel>() };
    copterModel->SetModel(cache->GetResource<Model>("Ghotiland/Models/Ozomcopter.mdl"));
    copterModel->SetMaterial(cache->GetResource<Material>("Materials/VCol.xml"));
    ozomcopterNode->SetPosition({ 32.f, 9.f, 30.f });
    ozomcopterNode->Yaw(23.f);
    copterModel->SetCastShadows(true);

    ozomcopterNode->CreateComponent<RigidBody>()->SetMass(0.f);
    ozomcopterNode->CreateComponent<CollisionShape>()->SetConvexHull(cache->GetResource<Model>("Ghotiland/Models/Ozomcopter_COLLIDER.mdl"));
    ozomcopterNode->CreateComponent<CollisionShape>()->SetTriangleMesh(cache->GetResource<Model>("Ghotiland/Models/Rotor_COLLIDER.mdl"));
}

void CharacterDemo::CreateCharacter()
{
    // character
    Node* objectNode{ scene_->CreateChild("Ozom") };
    objectNode->SetPosition({ 28.0f, 8.0f, -4.0f });
    character_ = objectNode->CreateComponent<Character>();

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Create the rendering component + animation controller
    Node* graphicsNode{ objectNode->CreateChild("Graphics") };
    AnimatedModel* object{ graphicsNode->CreateComponent<AnimatedModel>() };
    object->SetModel(cache->GetResource<Model>("Ghotiland/Models/Ozom.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Ghotiland/Materials/Ozom.xml"));
    object->SetCastShadows(true);
    graphicsNode->CreateComponent<AnimationController>();

    // Create rigidbody
    RigidBody* body{ objectNode->CreateComponent<RigidBody>() };
    body->SetCollisionLayerAndMask(ColLayer_Character, ColMask_Character);
    body->SetKinematic(true);
    body->SetTrigger(true);
    body->SetAngularFactor(Vector3::ZERO);
    body->SetCollisionEventMode(COLLISION_ALWAYS);
}

void CharacterDemo::CreateInstructions()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    UI* ui{ GetSubsystem<UI>() };

    // Construct new Text object, set string to display and font to use
    Text* instructionText{ ui->GetRoot()->CreateChild<Text>() };
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Philosopher.ttf"), 12);
    instructionText->SetTextAlignment(HA_CENTER);
    instructionText->SetText("WASD to move, Spacebar to Jump\nM to toggle debug");

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetPosition(0, 10);
}

void CharacterDemo::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(CharacterDemo, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, DRY_HANDLER(CharacterDemo, HandlePostUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(CharacterDemo, HandlePostRenderUpdate));
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    Camera* mainCamera{ cameraNode_->GetComponent<Camera>() };
    Camera* reflectCam{ cameraNode_->GetChild("Reflection")->GetComponent<Camera>() };
    if (reflectCam->GetAspectRatio() != mainCamera->GetAspectRatio())
        reflectCam->SetAspectRatio(mainCamera->GetAspectRatio());

    using namespace Update;

    Input* input{ GetSubsystem<Input>() };

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP | CTRL_RUN, false);

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
                character_->controls_.Set(CTRL_BACK,  input->GetKeyDown(KEY_S));
                character_->controls_.Set(CTRL_LEFT,  input->GetKeyDown(KEY_A));
                character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            }
            character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
            character_->controls_.Set(CTRL_RUN,  input->GetKeyDown(KEY_SHIFT));

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
                character_->controls_.yaw_   += input->GetMouseMoveX() * YAW_SENSITIVITY;
                character_->controls_.pitch_ += input->GetMouseMoveY() * YAW_SENSITIVITY;
            }

            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);

            // Turn on/off gyroscope on mobile platform
            if (touch_ && input->GetKeyPress(KEY_G))
                touch_->useGyroscope_ = !touch_->useGyroscope_;
        }
    }

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_M))
        drawDebug_ = !drawDebug_;
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode{ character_->GetNode() };
    const Quaternion rot{ character_->controls_.yaw_, Vector3::UP };
    const Quaternion dir{ rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT) };

    {
        const Vector3 aimPoint{ characterNode->GetPosition() + rot * Vector3{ 0.0f, 1.7f, 0.0f } };
        const Vector3 rayDir{ dir * Vector3::BACK };
        float rayDistance{ (touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST) };
        PhysicsRaycastResult result;

        scene_->GetComponent<PhysicsWorld>()->SphereCast(result, { aimPoint, rayDir }, .5f,
                                                         rayDistance, ColMask_Camera);
        if (result.body_)
            rayDistance = Min(rayDistance, result.distance_);
        rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

        cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
        cameraNode_->SetRotation(dir);
    }
}


void CharacterDemo::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (drawDebug_)
    {
        DebugRenderer* dbgRenderer{ scene_->GetComponent<DebugRenderer>() };
        Node* objectNode{ scene_->GetChild("Player") };

        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);

        if (objectNode)
            dbgRenderer->AddSphere(Sphere(objectNode->GetWorldPosition(), 0.1f), Color::YELLOW);
    }
}

