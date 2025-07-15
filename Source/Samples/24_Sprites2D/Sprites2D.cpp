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
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/2D/AnimatedSprite2D.h>
#include <Dry/2D/AnimationSet2D.h>
#include <Dry/2D/Sprite2D.h>

#include "Sprites2D.h"

#include <Dry/DebugNew.h>

// Number of static sprites to draw
static const unsigned NUM_SPRITES{ 200 };
static const StringHash VAR_MOVESPEED{ "MoveSpeed" };
static const StringHash VAR_ROTATESPEED{ "RotateSpeed" };

DRY_DEFINE_APPLICATION_MAIN(Sprites2D)

Sprites2D::Sprites2D(Context* context): Sample(context)
{
}

void Sprites2D::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASD keys to move and QE to zoom");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Sprites2D::CreateScene()
{
    scene_ = new Scene{ context_ };
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition({ 0.0f, 0.0f, -10.0f });

    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetOrthographic(true);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    camera->SetOrthoSize(graphics->GetHeight() * PIXEL_SIZE);

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Get sprite
    Sprite2D* sprite{ cache->GetResource<Sprite2D>("2D/Aster.png") };

    if (!sprite)
        return;

    const float halfWidth{ graphics->GetWidth() * 0.5f * PIXEL_SIZE };
    const float halfHeight{ graphics->GetHeight() * 0.5f * PIXEL_SIZE };

    for (unsigned i{ 0 }; i < NUM_SPRITES; ++i)
    {
        SharedPtr<Node> spriteNode{ scene_->CreateChild("StaticSprite2D") };
        spriteNode->SetPosition({ Random(-halfWidth, halfWidth), Random(-halfHeight, halfHeight), 0.0f });

        StaticSprite2D* staticSprite{ spriteNode->CreateComponent<StaticSprite2D>() };
        // Set random color
        staticSprite->SetColor({ Random(1.0f), Random(1.0f), Random(1.0f), 1.0f });
        // Set blend mode
        staticSprite->SetBlendMode(BLEND_ALPHA);
        // Set sprite
        staticSprite->SetSprite(sprite);

        // Set move speed
        spriteNode->SetVar(VAR_MOVESPEED, Vector3{ Random(-2.0f, 2.0f), Random(-2.0f, 2.0f), 0.0f });
        // Set rotate speed
        spriteNode->SetVar(VAR_ROTATESPEED, Random(-90.0f, 90.0f));

        // Add to sprite node vector
        spriteNodes_.Push(spriteNode);
    }

    // Get animation set
    AnimationSet2D* animationSet{ cache->GetResource<AnimationSet2D>("2D/GoldIcon.scml") };

    if (!animationSet)
        return;

    SharedPtr<Node> spriteNode(scene_->CreateChild("AnimatedSprite2D"));
    spriteNode->SetPosition({ 0.0f, 0.0f, -1.0f });

    AnimatedSprite2D* animatedSprite{ spriteNode->CreateComponent<AnimatedSprite2D>() };
    // Set animation
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("idle");
}

void Sprites2D::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void Sprites2D::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input{ GetSubsystem<Input>() };

    // Movement speed as world units per second
    const float MOVE_SPEED{ 4.0f };

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_Q))
    {
        Camera* camera{ cameraNode_->GetComponent<Camera>() };
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_E))
    {
        Camera* camera{ cameraNode_->GetComponent<Camera>() };
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Sprites2D::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(Sprites2D, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void Sprites2D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    const float halfWidth{ graphics->GetWidth() * 0.5f * PIXEL_SIZE };
    const float halfHeight{ graphics->GetHeight() * 0.5f * PIXEL_SIZE };

    for (unsigned i{ 0 }; i < spriteNodes_.Size(); ++i)
    {
        SharedPtr<Node> node{ spriteNodes_[i] };

        const Vector3 position{ node->GetPosition() };
        Vector3 moveSpeed{ node->GetVar(VAR_MOVESPEED).GetVector3() };
        Vector3 newPosition{ position + moveSpeed * timeStep };

        if (newPosition.x_ < -halfWidth || newPosition.x_ > halfWidth)
        {
            newPosition.x_ = position.x_;
            moveSpeed.x_ = -moveSpeed.x_;
            node->SetVar(VAR_MOVESPEED, moveSpeed);
        }
        if (newPosition.y_ < -halfHeight || newPosition.y_ > halfHeight)
        {
            newPosition.y_ = position.y_;
            moveSpeed.y_ = -moveSpeed.y_;
            node->SetVar(VAR_MOVESPEED, moveSpeed);
        }

        node->SetPosition(newPosition);

        const float rotateSpeed{ node->GetVar(VAR_ROTATESPEED).GetFloat() };
        node->Roll(rotateSpeed * timeStep);
    }
}
