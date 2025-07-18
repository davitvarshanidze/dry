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
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>

#include "SceneAndUILoad.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(SceneAndUILoad)

SceneAndUILoad::SceneAndUILoad(Context* context) :
    Sample(context)
{
}

void SceneAndUILoad::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Subscribe to global events for camera movement
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void SceneAndUILoad::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Load scene content prepared in the editor (XML format). GetFile() returns an open file from the resource system
    // which scene.LoadXML() will read
    SharedPtr<File> file{ cache->GetFile("Scenes/SceneLoadExample.xml") };
    scene_->LoadXML(*file);

    // Create the camera (not included in the scene file)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition({ 0.0f, 2.0f, -10.0f });
}

void SceneAndUILoad::CreateUI()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Graphics* graphics{ GetSubsystem<Graphics>() };
    UI* ui{ GetSubsystem<UI>() };

    // Set up global UI style into the root UI element
    XMLFile* style{ cache->GetResource<XMLFile>("UI/DefaultStyle.xml") };
    ui->GetRoot()->SetDefaultStyle(style);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will interact with the UI
    SharedPtr<Cursor> cursor{ new Cursor{ context_ } };
    cursor->SetStyleAuto();
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Load UI content prepared in the editor and add to the UI hierarchy
    SharedPtr<UIElement> layoutRoot{ ui->LoadLayout(cache->GetResource<XMLFile>("UI/UILoadExample.xml")) };
    ui->GetRoot()->AddChild(layoutRoot);

    // Subscribe to button actions (toggle scene lights when pressed then released)
    Button* button{ layoutRoot->GetChildStaticCast<Button>("ToggleLight1", true) };

    if (button)
        SubscribeToEvent(button, E_RELEASED, DRY_HANDLER(SceneAndUILoad, ToggleLight1));

    button = layoutRoot->GetChildStaticCast<Button>("ToggleLight2", true);

    if (button)
        SubscribeToEvent(button, E_RELEASED, DRY_HANDLER(SceneAndUILoad, ToggleLight2));
}

void SceneAndUILoad::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void SceneAndUILoad::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for camera motion
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(SceneAndUILoad, HandleUpdate));
}

void SceneAndUILoad::MoveCamera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    UI* ui{ GetSubsystem<UI>() };
    Input* input{ GetSubsystem<Input>() };
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element
    if (ui->GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED{ 20.0f };
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY{ 0.1f };

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        const IntVector2 mouseMove{ input->GetMouseMove() };
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

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

void SceneAndUILoad::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void SceneAndUILoad::ToggleLight1(StringHash eventType, VariantMap& eventData)
{
    Node* lightNode{ scene_->GetChild("Light1", true) };

    if (lightNode)
        lightNode->SetEnabled(!lightNode->IsEnabled());
}

void SceneAndUILoad::ToggleLight2(StringHash eventType, VariantMap& eventData)
{
    Node* lightNode{ scene_->GetChild("Light2", true) };

    if (lightNode)
        lightNode->SetEnabled(!lightNode->IsEnabled());
}
