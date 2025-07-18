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
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/2D/Sprite2D.h>
#include <Dry/2D/StaticSprite2D.h>
#include <Dry/2D/StretchableSprite2D.h>

#include "StretchableSprites2D.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(StretchableSprites2D)

StretchableSprites2D::StretchableSprites2D(Context* context): Sample(context),
    selectTransform_{ 0 }
{
}

void StretchableSprites2D::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASD keys to transform, Tab key to cycle through\n"
                       "Scale, Rotate, and Translate transform modes. In Rotate\n"
                       "mode, combine A/D keys with Ctrl key to rotate about\n"
                       "the Z axis");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void StretchableSprites2D::CreateScene()
{
    scene_ = new Scene{ context_ };
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetOrthographic(true);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    camera->SetOrthoSize(graphics->GetHeight() * PIXEL_SIZE);

    refSpriteNode_ = scene_->CreateChild("regular sprite");
    stretchSpriteNode_ = scene_->CreateChild("stretchable sprite");

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Sprite2D* sprite{ cache->GetResource<Sprite2D>("2D/Stretchable.png") };

    if (sprite)
    {
        refSpriteNode_->CreateComponent<StaticSprite2D>()->SetSprite(sprite);

        StretchableSprite2D* stretchSprite{ stretchSpriteNode_->CreateComponent<StretchableSprite2D>() };
        stretchSprite->SetSprite(sprite);
        stretchSprite->SetBorder({25, 25, 25, 25});

        refSpriteNode_->Translate2D(Vector2{ -2.0f, 0.0f });
        stretchSpriteNode_->Translate2D(Vector2{ 2.0f, 0.0f });
    }
}

void StretchableSprites2D::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void StretchableSprites2D::SubscribeToEvents()
{
    SubscribeToEvent(E_KEYUP, DRY_HANDLER(StretchableSprites2D, OnKeyUp));

    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(StretchableSprites2D, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void StretchableSprites2D::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    switch (selectTransform_)
    {
    case 0: ScaleSprites(timeStep);
        break;
    case 1: RotateSprites(timeStep);
        break;
    case 2: TranslateSprites(timeStep);
        break;
    default: DRY_LOGERRORF("bad transform selection: %d", selectTransform_);
    }
}

void StretchableSprites2D::OnKeyUp(StringHash /*eventType*/, VariantMap& eventData)
{
    const int key{ eventData[KeyUp::P_KEY].GetInt() };

    if (key == KEY_TAB)
    {
        ++selectTransform_;
        selectTransform_ %= 3;
    }
    else if (key == KEY_ESCAPE)
    {
        engine_->Exit();
    }
}


void StretchableSprites2D::TranslateSprites(float timeStep)
{
    Input* input{ GetSubsystem<Input>() };
    const bool left{  input->GetKeyDown(KEY_A) };
    const bool right{ input->GetKeyDown(KEY_D) };
    const bool up{    input->GetKeyDown(KEY_W) };
    const bool down{  input->GetKeyDown(KEY_S) };
    const float speed{ 1.0f };

    if (left || right || up || down)
    {
        const float quantum{ timeStep * speed };
        const Vector2 translate{ (left ? -quantum : 0.0f) + (right ? quantum : 0.0f),
                                 (down ? -quantum : 0.0f) + (up ? quantum : 0.0f) };

        refSpriteNode_->Translate2D(translate);
        stretchSpriteNode_->Translate2D(translate);
    }
}

void StretchableSprites2D::RotateSprites(float timeStep)
{

    Input* input{ GetSubsystem<Input>() };
    const bool left{  input->GetKeyDown(KEY_A) };
    const bool right{ input->GetKeyDown(KEY_D) };
    const bool up{    input->GetKeyDown(KEY_W) };
    const bool down{  input->GetKeyDown(KEY_S) };
    const bool ctrl{  input->GetKeyDown(KEY_CTRL) };
    const float speed{ 45.0f };

    if (left || right || up || down)
    {
        const float quantum{ timeStep * speed };

        const float xrot{ (up ? -quantum : 0.0f) + (down ? quantum : 0.0f) };
        const float rot2{ (left ? -quantum : 0.0f) + (right ? quantum : 0.0f) };
        const Quaternion totalRot{ xrot, ctrl ? 0.0f : rot2, ctrl ? rot2 : 0.0f };

        refSpriteNode_->Rotate(totalRot);
        stretchSpriteNode_->Rotate(totalRot);
    }
}

void StretchableSprites2D::ScaleSprites(float timeStep)
{
    Input* input{ GetSubsystem<Input>() };
    const bool left{  input->GetKeyDown(KEY_A) };
    const bool right{ input->GetKeyDown(KEY_D) };
    const bool up{    input->GetKeyDown(KEY_W) };
    const bool down{  input->GetKeyDown(KEY_S) };
    const float speed{ 0.5f };

    if (left || right || up || down)
    {
        const float quantum{ timeStep * speed };
        const Vector2 scale{ 1.0f + (right ? quantum : left ? -quantum : 0.0f),
                             1.0f + (up ? quantum : down ? -quantum : 0.0f) };

        refSpriteNode_->Scale2D(scale);
        stretchSpriteNode_->Scale2D(scale);
    }
}
