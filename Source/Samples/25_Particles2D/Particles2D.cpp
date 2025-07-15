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
#include <Dry/2D/ParticleEffect2D.h>
#include <Dry/2D/ParticleEmitter2D.h>

#include "Particles2D.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(Particles2D)

Particles2D::Particles2D(Context* context): Sample(context)
{
}

void Particles2D::Start()
{
    // Execute base class startup
    Sample::Start();

    // Set mouse visible
    Input* input{ GetSubsystem<Input>() };
    input->SetMouseVisible(true);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Particles2D::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetOrthographic(true);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.2f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    ParticleEffect2D* particleEffect{ cache->GetResource<ParticleEffect2D>("2D/sun.pex") };
    if (!particleEffect)
        return;

    particleNode_ = scene_->CreateChild("ParticleEmitter2D");
    ParticleEmitter2D* particleEmitter{ particleNode_->CreateComponent<ParticleEmitter2D>() };
    particleEmitter->SetEffect(particleEffect);

    ParticleEffect2D* greenSpiralEffect{ cache->GetResource<ParticleEffect2D>("2D/greenspiral.pex") };
    if (!greenSpiralEffect)
        return;

    Node* greenSpiralNode{ scene_->CreateChild("GreenSpiral") };
    ParticleEmitter2D* greenSpiralEmitter{ greenSpiralNode->CreateComponent<ParticleEmitter2D>() };
    greenSpiralEmitter->SetEffect(greenSpiralEffect);
}

void Particles2D::CreateInstructions()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    UI* ui{ GetSubsystem<UI>() };

    // Construct new Text object, set string to display and font to use
    Text* instructionText{ ui->GetRoot()->CreateChild<Text>() };
    instructionText->SetText("Use mouse/touch to move the particle.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Particles2D::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Particles2D::SubscribeToEvents()
{
    SubscribeToEvent(E_MOUSEMOVE, DRY_HANDLER(Particles2D, HandleMouseMove));
    if (touchEnabled_)
        SubscribeToEvent(E_TOUCHMOVE, DRY_HANDLER(Particles2D, HandleMouseMove));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void Particles2D::HandleMouseMove(StringHash /*eventType*/, VariantMap& eventData)
{
    if (particleNode_)
    {
        using namespace MouseMove;

        Graphics* graphics{ GetSubsystem<Graphics>() };
        Camera* camera{ cameraNode_->GetComponent<Camera>() };
        const IntVector2 screenPoint{ eventData[P_X].GetInt(), eventData[P_Y].GetInt() };

        particleNode_->SetPosition(camera->ScreenToWorldPos(
                                       graphics->NormalizedScreenPos(screenPoint), 10.0f));
    }
}
