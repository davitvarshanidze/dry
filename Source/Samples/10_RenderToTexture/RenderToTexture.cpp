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
#include <Dry/Graphics/RenderSurface.h>
#include <Dry/Graphics/ReflectionProbe.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Technique.h>
#include <Dry/Graphics/Texture2D.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "RenderToTexture.h"
#include "Rotator.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(RenderToTexture)

RenderToTexture::RenderToTexture(Context* context) :
    Sample(context)
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    context->RegisterFactory<Rotator>();
}

void RenderToTexture::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void RenderToTexture::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    {
        // Create the scene which will be rendered to a texture
        rttScene_ = new Scene{ context_ };

        // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
        rttScene_->CreateComponent<Octree>();

        // Create a Zone for ambient light & fog control
        Node* zoneNode{ rttScene_->CreateChild("Zone") };
        Zone* zone{ zoneNode->CreateComponent<Zone>() };
        // Set same volume as the Octree, set a close bluish fog and some ambient light
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor({ 0.1f, 0.05f, 0.4f });
        zone->SetFogColor({ 0.125f, 0.0f, 0.15f });
        zone->SetFogStart(9.0f);
        zone->SetFogEnd(42.0f);

        // Create randomly positioned and oriented box StaticModels in the scene
        const unsigned NUM_OBJECTS{ 1000 };

        for (unsigned i{ 0 }; i < NUM_OBJECTS; ++i)
        {
            Node* boxNode{ rttScene_->CreateChild("Box") };
            boxNode->SetPosition(Vector3(Random(50.0f) - 25.0f, Random(50.0f) - 30.0f, Random(50.0f)));
            // Orient using random pitch, yaw and roll Euler angles
            boxNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
            StaticModel* boxObject{ boxNode->CreateComponent<StaticModel>() };
            boxObject->SetModel(cache->GetResource<Model>("Models/FancyBox.mdl"));
            boxObject->SetMaterial(cache->GetResource<Material>("Materials/GrassRock.xml"));

            // Add our custom Rotator component which will rotate the scene node each frame, when the scene sends its update event.
            // Simply set same rotation speed for all objects
            Rotator* rotator{ boxNode->CreateComponent<Rotator>() };
            rotator->SetRotationSpeed(Vector3(10.0f, 20.0f, 30.0f));
        }

        // Create a camera for the render-to-texture scene. Simply leave it at the world origin and let it observe the scene
        rttCameraNode_ = rttScene_->CreateChild("Camera");
        Camera* camera{ rttCameraNode_->CreateComponent<Camera>() };
        camera->SetFarClip(42.0f);

        // Create a point light to the camera scene node
        Light* light{ rttCameraNode_->CreateComponent<Light>() };
        light->SetLightType(LIGHT_POINT);
        light->SetRange(32.0f);
        light->SetBrightness(1.23f);
        light->SetColor({ 0.7f, 1.0f, 0.4f });

        // Create a reflective cone in the background
        Node* coneNode{ rttScene_->CreateChild("Cone") };
        coneNode->SetPosition({ 0.f, 0.f, 25.f });
        coneNode->SetScale({ 50.f, 1.f, 50.f });
        coneNode->Pitch(-97.f);
        StaticModel* coneObject{ coneNode->CreateComponent<StaticModel>() };
        coneObject->SetModel(cache->GetResource<Model>("Models/Cone.mdl"));
        coneObject->SetMaterial(cache->GetResource<Material>("Materials/ChromeEnvMap.xml")->Clone());
        context_->RegisterFactory<ReflectionProbe>();
        ReflectionProbe* probe{ coneNode->CreateComponent<ReflectionProbe>() };
        probe->SetResolution(256);
        probe->SetFarClip(55.f);
        Material* mat{ coneObject->GetMaterial() };
        mat->SetTexture(TU_ENVIRONMENT, probe->GetReflectionMap());
        mat->SetShaderParameter("MatEnvMapColor", Color::CHARTREUSE * 2.3f);
        mat->SetShaderParameter("MatSpecColor", Color::TRANSPARENT_BLACK);
    }

    {
        // Create the scene in which we move around
        scene_ = new Scene{ context_ };

        // Create octree, use also default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
        scene_->CreateComponent<Octree>();

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
        light->SetColor({ .2f });

        // Create a "floor" consisting of several tiles
        for (int y{ -5 }; y <= 5; ++y)
        {
            for (int x{ -5 }; x <= 5; ++x)
            {
                Node* floorNode = scene_->CreateChild("FloorTile");
                floorNode->SetPosition(Vector3(x * 20.5f, -0.5f, y * 20.5f));
                floorNode->SetScale(Vector3(20.0f, 1.0f, 20.f));
                auto* floorObject = floorNode->CreateComponent<StaticModel>();
                floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
                floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
            }
        }

        // Create a "screen" like object for viewing the second scene. Construct it from two StaticModels, a box for the frame
        // and a plane for the actual view
        {
            Node* boxNode = scene_->CreateChild("ScreenBox");
            boxNode->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
            boxNode->SetScale(Vector3(21.0f, 16.0f, 0.5f));
            auto* boxObject = boxNode->CreateComponent<StaticModel>();
            boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
            boxObject->SetCastShadows(true);

            Node* screenNode = scene_->CreateChild("Screen");
            screenNode->SetPosition(Vector3(0.0f, 10.0f, -0.27f));
            screenNode->SetRotation(Quaternion(-90.0f, 0.0f, 0.0f));
            screenNode->SetScale(Vector3(20.0f, 0.0f, 15.0f));
            auto* screenObject = screenNode->CreateComponent<StaticModel>();
            screenObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));

            // Create a renderable texture (1024x768, RGB format), enable bilinear filtering on it
            SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
            renderTexture->SetSize(1024, 768, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
            renderTexture->SetFilterMode(FILTER_BILINEAR);

            // Create a new material from scratch, use the diffuse unlit technique, assign the render texture
            // as its diffuse texture, then assign the material to the screen plane object
            SharedPtr<Material> renderMaterial(new Material(context_));
            renderMaterial->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffUnlit.xml"));
            renderMaterial->SetTexture(TU_DIFFUSE, renderTexture);
            // Since the screen material is on top of the box model and may Z-fight, use negative depth bias
            // to push it forward (particularly necessary on mobiles with possibly less Z resolution)
            renderMaterial->SetDepthBias(BiasParameters(-1e-5f, 0.f));
            screenObject->SetMaterial(renderMaterial);

            // Get the texture's RenderSurface object (exists when the texture has been created in rendertarget mode)
            // and define the viewport for rendering the second scene, similarly as how backbuffer viewports are defined
            // to the Renderer subsystem. By default the texture viewport will be updated when the texture is visible
            // in the main view
            RenderSurface* surface = renderTexture->GetRenderSurface();
            surface->SetUpdateMode(SURFACE_UPDATEALWAYS);
            SharedPtr<Viewport> rttViewport(new Viewport(context_, rttScene_, rttCameraNode_->GetComponent<Camera>()));
            surface->SetViewport(0, rttViewport);
        }

        // Create hovering reflective objects.
        for (int i{ 0 }; i < 5; ++i)
        {
            Node* sphereNode{ scene_->CreateChild("Sphere") };
            sphereNode->SetPosition({ -20.f + 2.f * i, 3.f + (i % 2 - .5f * (i == 2)) * 5.f, -15.f + i * 3.f });
            sphereNode->SetScale(5.f);
            StaticModel* sphereObject{ sphereNode->CreateComponent<StaticModel>() };
            sphereObject->SetModel(cache->GetResource<Model>(String{ "Models/" } + (i == 2 ? "Teapot.mdl" : "Sphere.mdl" )));
            sphereObject->SetMaterial(cache->GetResource<Material>("Materials/ChromeEnvMap.xml")->Clone());
            ReflectionProbe* probe{ sphereNode->CreateComponent<ReflectionProbe>() };
            probe->SetResolution(PowN(2u, i * 2));
            probe->SetFarClip(100.f);
            Material* mat{ sphereObject->GetMaterial() };
            mat->SetTexture(TU_ENVIRONMENT, probe->GetReflectionMap());
            mat->SetShaderParameter("MatEnvMapColor", Color::WHITE);

            ValueAnimation* hover{ new ValueAnimation(context_) };
            hover->SetInterpolationMethod(IM_SINUSOIDAL);
            const Vector3 startPos{ sphereNode->GetPosition() };
            hover->SetKeyFrame(0.f, startPos);
            hover->SetKeyFrame(.5f, startPos + Vector3::UP);
            hover->SetKeyFrame(1.f, startPos);
            sphereNode->SetAttributeAnimation("Position", hover);
            sphereNode->SetAttributeAnimationSpeed("Position", .23f);
            sphereNode->SetAttributeAnimationTime("Position", i * .42f);

            if (i == 2)
            {
                probe->SetViewMask(1);
                sphereObject->SetViewMask(M_MAX_UNSIGNED - 1);

                ValueAnimation* spin{ new ValueAnimation(context_) };
                spin->SetKeyFrame(0.f, Quaternion::IDENTITY);
                spin->SetKeyFrame(.5f, Quaternion{ 180.f, Vector3::UP });
                spin->SetKeyFrame(1.f, Quaternion{ 360.f, Vector3::UP });
                sphereNode->SetAttributeAnimation("Rotation", spin);
                sphereNode->SetAttributeAnimationSpeed("Rotation", .17f);
            }
        }

        // Create the camera which we will move around. Limit far clip distance to match the fog
        cameraNode_ = scene_->CreateChild("Camera");
        auto* camera = cameraNode_->CreateComponent<Camera>();
        camera->SetFarClip(300.0f);

        // Set an initial position for the camera scene node above the plane
        cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -30.0f));
    }
}

void RenderToTexture::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void RenderToTexture::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
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
}

void RenderToTexture::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(RenderToTexture, HandleUpdate));
}

void RenderToTexture::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}
