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
#include <Dry/Graphics/Camera.h>
#include <Dry/Engine/Engine.h>
#include <Dry/UI/Font.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Input/Input.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Text.h>
#include <Dry/2D/StaticSprite2D.h>
#include <Dry/2D/TileMap2D.h>
#include <Dry/2D/TileMapLayer2D.h>
#include <Dry/2D/TmxFile2D.h>

#include "TiledMap2D.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(TiledMap2D)

TiledMap2D::TiledMap2D(Context* context): Sample(context)
{
}

void TiledMap2D::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<UI>()->SetCursor(new Cursor{ context_ });
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASD keys to move and QE to zoom\n"
                       "LMB to remove a tile, RMB to swap grass and water");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void TiledMap2D::CreateScene()
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
    camera->SetZoom(Min(graphics->GetWidth() / 1280.0f, graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.0) is set for full visibility at 1280x800 resolution)

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Get tmx file
    TmxFile2D* tmxFile{ cache->GetResource<TmxFile2D>("2D/isometric_grass_and_water.tmx") };

    if (!tmxFile)
        return;

    SharedPtr<Node> tileMapNode(scene_->CreateChild("TileMap"));
    tileMapNode->SetPosition(Vector3::BACK);

    TileMap2D* tileMap{ tileMapNode->CreateComponent<TileMap2D>() };
    // Set animation
    tileMap->SetTmxFile(tmxFile);

    // Set camera's position
    const TileMapInfo2D& info{ tileMap->GetInfo() };

    const float x{ info.GetMapWidth() * 0.5f };
    const float y{ info.GetMapHeight() * 0.5f };
    cameraNode_->SetPosition(Vector3(x, y, -10.0f));
}

void TiledMap2D::SetupViewport()
{
    Renderer* renderer{ GetSubsystem<Renderer>() };

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);
}

void TiledMap2D::MoveCamera(float timeStep)
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

void TiledMap2D::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(TiledMap2D, HandleUpdate));

    // Listen to mouse clicks
    SubscribeToEvent(E_MOUSEBUTTONDOWN, DRY_HANDLER(TiledMap2D, HandleMouseButtonDown));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void TiledMap2D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void TiledMap2D::HandleMouseButtonDown(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    Input* input{ GetSubsystem<Input>() };

    Node* tileMapNode{ scene_->GetChild("TileMap", true) };
    TileMap2D* map{ tileMapNode->GetComponent<TileMap2D>() };
    TileMapLayer2D* layer{ map->GetLayer(0) };

    Vector2 pos{ GetMousePositionXY() };
    int x, y;

    if (map->PositionToTileIndex(x, y, pos))
    {
        // Get tile's sprite. Note that layer.GetTile(x, y).sprite is read-only, so we get the sprite through tile's node
        Node* n{ layer->GetTileNode(x, y) };

        if (!n)
            return;

        StaticSprite2D* sprite{ n->GetComponent<StaticSprite2D>() };

        if (input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            // Swap grass and water
            if (layer->GetTile(x, y)->GetGid() < 9) // First 8 sprites in the "isometric_grass_and_water.png" tileset are mostly grass and from 9 to 24 they are mostly water
                sprite->SetSprite(layer->GetTile(0, 0)->GetSprite()); // Replace grass by water sprite used in top tile
            else
                sprite->SetSprite(layer->GetTile(24, 24)->GetSprite()); // Replace water by grass sprite used in bottom tile
        }
        else
        {
            sprite->SetSprite(nullptr); // 'Remove' sprite
        }
    }
}
