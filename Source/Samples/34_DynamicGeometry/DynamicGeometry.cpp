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
#include <Dry/Core/Profiler.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/Geometry.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/IndexBuffer.h>
#include <Dry/Graphics/Light.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/VertexBuffer.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/Log.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "DynamicGeometry.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(DynamicGeometry)

DynamicGeometry::DynamicGeometry(Context* context): Sample(context),
    animate_{ true },
    time_{ 0.0f }
{
}

void DynamicGeometry::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDEQ keys and mouse/touch to move\n"
                       "Space to toggle animation");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void DynamicGeometry::CreateScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };

    // Create the Octree component to the scene so that drawable objects can be rendered. Use default volume
    // (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->CreateComponent<Octree>();

    // Create a Zone for ambient light & fog control
    Node* zoneNode{ scene_->CreateChild("Zone") };
    Zone* zone{ zoneNode->CreateComponent<Zone>() };
    zone->SetBoundingBox(BoundingBox{ -1000.0f, 1000.0f });
    zone->SetFogColor({ 0.2f, 0.2f, 0.2f });
    zone->SetFogStart(200.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light
    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection({ -0.6f, -1.0f, -0.8f }); // The direction vector does not need to be normalized
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor({ 0.4f, 1.0f, 0.4f });
    light->SetSpecularIntensity(1.5f);

    // Get the original model and its unmodified vertices, which are used as source data for the animation
    Model* originalModel{ cache->GetResource<Model>("Models/Box.mdl") };

    if (!originalModel)
    {
        DRY_LOGERROR("Model not found, cannot initialize example scene");
        return;
    }

    // Get the vertex buffer from the first geometry's first LOD level
    VertexBuffer* buffer{ originalModel->GetGeometry(0, 0)->GetVertexBuffer(0) };
    const unsigned char* vertexData{ (const unsigned char*)buffer->Lock(0, buffer->GetVertexCount()) };

    if (vertexData)
    {
        const unsigned numVertices{ buffer->GetVertexCount() };
        const unsigned vertexSize{ buffer->GetVertexSize() };

        // Copy the original vertex positions
        for (unsigned i{ 0 }; i < numVertices; ++i)
        {
            const Vector3& src{ *reinterpret_cast<const Vector3*>(vertexData + i * vertexSize) };
            originalVertices_.Push(src);
        }

        buffer->Unlock();

        // Detect duplicate vertices to allow seamless animation
        vertexDuplicates_.Resize(originalVertices_.Size());
        for (unsigned i{ 0 }; i < originalVertices_.Size(); ++i)
        {
            vertexDuplicates_[i] = i; // Assume not a duplicate

            for (unsigned j{ 0 }; j < i; ++j)
            {
                if (originalVertices_[i].Equals(originalVertices_[j]))
                {
                    vertexDuplicates_[i] = j;
                    break;
                }
            }
        }
    }
    else
    {
        DRY_LOGERROR("Failed to lock the model vertex buffer to get original vertices");
        return;
    }

    // Create StaticModels in the scene. Clone the model for each so that we can modify the vertex data individually
    for (int y{ -1 }; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x)
    {
        Node* node{ scene_->CreateChild("Object") };
        node->SetPosition({ x * 2.0f, 0.0f, y * 2.0f });

        StaticModel* object{ node->CreateComponent<StaticModel>() };
        SharedPtr<Model> cloneModel{ originalModel->Clone() };
        object->SetModel(cloneModel);

        // Store the cloned vertex buffer that we will modify when animating
        animatingBuffers_.Push(SharedPtr<VertexBuffer>(cloneModel->GetGeometry(0, 0)->GetVertexBuffer(0)));
    }

    // Finally create one model (pyramid shape) and a StaticModel to display it from scratch
    // Note: there are duplicated vertices to enable face normals. We will calculate normals programmatically
    {
        const unsigned numVertices{ 18 };

        float vertexData[] = {
            // Position             Normal
            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f,

            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,

            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f
        };

        const unsigned short indexData[] = {
            0, 1, 2,
            3, 4, 5,
            6, 7, 8,
            9, 10, 11,
            12, 13, 14,
            15, 16, 17
        };

        // Calculate face normals now
        for (unsigned i{ 0 }; i < numVertices; i += 3)
        {
            Vector3& v1{ *(reinterpret_cast<Vector3*>(&vertexData[6 * i])) };
            Vector3& v2{ *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 1)])) };
            Vector3& v3{ *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 2)])) };
            Vector3& n1{ *(reinterpret_cast<Vector3*>(&vertexData[6 * i + 3])) };
            Vector3& n2{ *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 1) + 3])) };
            Vector3& n3{ *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 2) + 3])) };

            Vector3 edge1{ v1 - v2 };
            Vector3 edge2{ v1 - v3 };
            n1 = n2 = n3 = edge1.CrossProduct(edge2).Normalized();
        }

        SharedPtr<Model> fromScratchModel{ new Model{ context_ } };
        SharedPtr<VertexBuffer> vb{ new VertexBuffer{ context_ } };
        SharedPtr<IndexBuffer> ib{ new IndexBuffer{ context_ } };
        SharedPtr<Geometry> geom{ new Geometry{ context_ } };

        // Shadowed buffer needed for raycasts to work, and so that data can be automatically restored on device loss
        vb->SetShadowed(true);
        // We could use the "legacy" element bitmask to define elements for more compact code, but let's demonstrate
        // defining the vertex elements explicitly to allow any element types and order
        PODVector<VertexElement> elements;
        elements.Push(VertexElement{ TYPE_VECTOR3, SEM_POSITION });
        elements.Push(VertexElement{ TYPE_VECTOR3, SEM_NORMAL });
        vb->SetSize(numVertices, elements);
        vb->SetData(vertexData);

        ib->SetShadowed(true);
        ib->SetSize(numVertices, false);
        ib->SetData(indexData);

        geom->SetVertexBuffer(0, vb);
        geom->SetIndexBuffer(ib);
        geom->SetDrawRange(TRIANGLE_LIST, 0, numVertices);

        fromScratchModel->SetNumGeometries(1);
        fromScratchModel->SetGeometry(0, 0, geom);
        fromScratchModel->SetBoundingBox(BoundingBox{ -0.5f, 0.5f });

        // Though not necessary to render, the vertex & index buffers must be listed in the model so that it can be saved properly
        Vector<SharedPtr<VertexBuffer> > vertexBuffers;
        Vector<SharedPtr<IndexBuffer> > indexBuffers;
        vertexBuffers.Push(vb);
        indexBuffers.Push(ib);
        // Morph ranges could also be not defined. Here we simply define a zero range (no morphing) for the vertex buffer
        PODVector<unsigned> morphRangeStarts;
        PODVector<unsigned> morphRangeCounts;
        morphRangeStarts.Push(0);
        morphRangeCounts.Push(0);
        fromScratchModel->SetVertexBuffers(vertexBuffers, morphRangeStarts, morphRangeCounts);
        fromScratchModel->SetIndexBuffers(indexBuffers);

        Node* node{ scene_->CreateChild("FromScratchObject") };
        node->SetPosition({ 0.0f, 3.0f, 0.0f });
        StaticModel* object{ node->CreateComponent<StaticModel>() };
        object->SetModel(fromScratchModel);
    }

    // Create the camera
    cameraNode_ = new Node{ context_ };
    cameraNode_->SetPosition({ 0.0f, 2.0f, -20.0f });
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetFarClip(300.0f);
}

void DynamicGeometry::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void DynamicGeometry::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(DynamicGeometry, HandleUpdate));
}

void DynamicGeometry::MoveCamera(float timeStep)
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
}

void DynamicGeometry::AnimateObjects(float timeStep)
{
    DRY_PROFILE(AnimateObjects);

    time_ += timeStep * 100.0f;

    // Repeat for each of the cloned vertex buffers
    for (unsigned i{ 0 }; i < animatingBuffers_.Size(); ++i)
    {
        const float startPhase{ time_ + i * 30.0f };
        VertexBuffer* buffer{ animatingBuffers_[i] };

        // Lock the vertex buffer for update and rewrite positions with sine wave modulated ones
        // Cannot use discard lock as there is other data (normals, UVs) that we are not overwriting
        unsigned char* vertexData{ (unsigned char*)buffer->Lock(0, buffer->GetVertexCount()) };

        if (vertexData)
        {
            const unsigned vertexSize{ buffer->GetVertexSize() };
            const unsigned numVertices{ buffer->GetVertexCount() };

            for (unsigned j{ 0 }; j < numVertices; ++j)
            {
                // If there are duplicate vertices, animate them in phase of the original
                const float phase{ startPhase + vertexDuplicates_[j] * 10.0f };
                Vector3& src{ originalVertices_[j] };
                Vector3& dest{ *reinterpret_cast<Vector3*>(vertexData + j * vertexSize) };
                dest.x_ = src.x_ * (1.0f + 0.1f * Sin(phase));
                dest.y_ = src.y_ * (1.0f + 0.1f * Sin(phase + 60.0f));
                dest.z_ = src.z_ * (1.0f + 0.1f * Sin(phase + 120.0f));
            }

            buffer->Unlock();
        }
    }
}

void DynamicGeometry::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Toggle animation with space
    Input* input{ GetSubsystem<Input>() };

    if (input->GetKeyPress(KEY_SPACE))
        animate_ = !animate_;

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    // Animate objects' vertex data if enabled
    if (animate_)
        AnimateObjects(timeStep);
}
