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
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/ObjectAnimation.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Sprite.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "LightAnimation.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(LightAnimation)

LightAnimation::LightAnimation(Context* context): Sample(context)
{
}

void LightAnimation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the UI content
    CreateInstructions();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void LightAnimation::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();

    // Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
    // plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
    // (100 x 100 world units)
    Node* planeNode{ scene_->CreateChild("Plane") };
    planeNode->SetScale({ 100.0f, 1.0f, 100.0f });
    StaticModel* planeObject{ planeNode->CreateComponent<StaticModel>() };
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a point light to the world so that we can see something.
    Node* lightNode{ scene_->CreateChild("PointLight") };
    lightNode->SetPosition(Vector3::UP * 10.f);
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_SPOT);
    light->SetRange(30.0f);
    light->SetFov(110.f);

    // Create light animation
    SharedPtr<ObjectAnimation> lightAnimation{ new ObjectAnimation{ context_ } };

    // Create light position animation
    SharedPtr<ValueAnimation> rotationAnimation{ new ValueAnimation{ context_ } };
    // Use sinsoidal interpolation method
    rotationAnimation->SetInterpolationMethod(IM_SINUSOIDAL);
    rotationAnimation->SetKeyFrame(0.0f, Quaternion{  30.0f, Vector3::RIGHT });
    rotationAnimation->SetKeyFrame(2.0f, Quaternion{ 150.0f, Vector3::RIGHT });
    rotationAnimation->SetKeyFrame(4.0f, Quaternion{  30.0f, Vector3::RIGHT });
    // Set position animation
    lightAnimation->AddAttributeAnimation("Rotation", rotationAnimation);

    // Create text animation
    SharedPtr<ValueAnimation> textAnimation{ new ValueAnimation{ context_ } };
    textAnimation->SetKeyFrame(0.0f, "WHITE");
    textAnimation->SetKeyFrame(1.0f, "RED");
    textAnimation->SetKeyFrame(2.0f, "YELLOW");
    textAnimation->SetKeyFrame(3.0f, "GREEN");
    textAnimation->SetKeyFrame(4.0f, "WHITE");
    GetSubsystem<UI>()->GetRoot()->GetChild(String("animatingText"))->SetAttributeAnimation("Text", textAnimation);

    // Create UI element animation
    // (note: a spritesheet and "Image Rect" attribute should be used in real use cases for better performance)
    SharedPtr<ValueAnimation> spriteAnimation{ new ValueAnimation{ context_ } };
    spriteAnimation->SetKeyFrame(0.0f, ResourceRef("Texture2D", "2D/GoldIcon/1.png"));
    spriteAnimation->SetKeyFrame(0.1f, ResourceRef("Texture2D", "2D/GoldIcon/2.png"));
    spriteAnimation->SetKeyFrame(0.2f, ResourceRef("Texture2D", "2D/GoldIcon/3.png"));
    spriteAnimation->SetKeyFrame(0.3f, ResourceRef("Texture2D", "2D/GoldIcon/4.png"));
    spriteAnimation->SetKeyFrame(0.4f, ResourceRef("Texture2D", "2D/GoldIcon/5.png"));
    spriteAnimation->SetKeyFrame(0.5f, ResourceRef("Texture2D", "2D/GoldIcon/1.png"));
    SharedPtr<ValueAnimation> hoverAnimation{ new ValueAnimation{ context_ } };
    hoverAnimation->SetInterpolationMethod(IM_SINUSOIDAL);
    hoverAnimation->SetKeyFrame(0.f, Vector2(8, 8));
    hoverAnimation->SetKeyFrame(1.f, Vector2(8, 32));
    hoverAnimation->SetKeyFrame(2.f, Vector2(8, 8));

    UIElement* coin{ GetSubsystem<UI>()->GetRoot()->GetChild(String("animatingSprite")) };
    coin->SetAttributeAnimation("Texture", spriteAnimation);
    coin->SetAttributeAnimation("Position", hoverAnimation);

    // Create light color animation
    SharedPtr<ValueAnimation> colorAnimation{ new ValueAnimation{ context_ } };
    colorAnimation->SetKeyFrame(0.0f, Color::WHITE);
    colorAnimation->SetKeyFrame(1.0f, Color::RED);
    colorAnimation->SetKeyFrame(2.0f, Color::YELLOW);
    colorAnimation->SetKeyFrame(3.0f, Color::GREEN);
    colorAnimation->SetKeyFrame(4.0f, Color::WHITE);
    // Set Light component's color animation
    lightAnimation->AddAttributeAnimation("@Light/Color", colorAnimation);

    // Apply light animation to light node
    lightNode->SetObjectAnimation(lightAnimation);

    // Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
    // quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
    // LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
    // see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
    // same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
    // scene.
    const unsigned NUM_OBJECTS{ 200 };

    for (unsigned i{ 0 }; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode{ scene_->CreateChild("Mushroom") };
        mushroomNode->SetPosition({ Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f });
        mushroomNode->SetRotation(Quaternion{ 0.0f, Random(360.0f), 0.0f });
        mushroomNode->SetScale(0.5f + Random(2.0f));

        StaticModel* mushroomObject{ mushroomNode->CreateComponent<StaticModel>() };
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, -50.0f));
}

void LightAnimation::CreateInstructions()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    UI* ui{ GetSubsystem<UI>() };

    // Construct new Text object, set string to display and font to use
    Text* instructionText{ ui->GetRoot()->CreateChild<Text>() };
    instructionText->SetText("Use WASDEQ keys and mouse/touch to move");
    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };
    instructionText->SetFont(font, 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);

    // Animating text
    Text* text{ ui->GetRoot()->CreateChild<Text>("animatingText") };
    text->SetFont(font, 15);
    text->SetHorizontalAlignment(HA_CENTER);
    text->SetVerticalAlignment(VA_CENTER);
    text->SetPosition(0, ui->GetRoot()->GetHeight() / 4 + 20);

    // Animating sprite in the top left corner
    Sprite* sprite{ ui->GetRoot()->CreateChild<Sprite>("animatingSprite") };
    sprite->SetPosition(8, 8);
    sprite->SetSize(64, 64);
}

void LightAnimation::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void LightAnimation::MoveCamera(float timeStep)
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
    // Use the Translate() function (default local space) to move relative to the node's orientation.
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

void LightAnimation::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(LightAnimation, HandleUpdate));
}

void LightAnimation::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}
