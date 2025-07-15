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

#include <Dry/2D/AnimatedSprite2D.h>
#include <Dry/2D/AnimationSet2D.h>
#include <Dry/UI/Button.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/2D/CollisionBox2D.h>
#include <Dry/2D/CollisionChain2D.h>
#include <Dry/2D/CollisionCircle2D.h>
#include <Dry/2D/CollisionPolygon2D.h>
#include <Dry/Core/CoreEvents.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Engine/Engine.h>
#include <Dry/UI/Font.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/GraphicsEvents.h>
#include <Dry/Input/Input.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/2D/PhysicsWorld2D.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/2D/RigidBody2D.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Text.h>
#include <Dry/2D/TileMap2D.h>
#include <Dry/2D/TileMapLayer2D.h>
#include <Dry/2D/TmxFile2D.h>
#include <Dry/UI/UIEvents.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/2D/PhysicsEvents2D.h>

#include <Dry/DebugNew.h>

#include "Character2D.h"
#include "Utilities2D/Sample2D.h"
#include "Utilities2D/Mover.h"
#include "IsometricDemo2D.h"

DRY_DEFINE_APPLICATION_MAIN(IsometricDemo2D)

IsometricDemo2D::IsometricDemo2D(Context* context): Sample(context),
    zoom_{ 2.0f },
    drawDebug_{ false }
{
    // Register factory for the Character2D component so it can be created via CreateComponent
    Character2D::RegisterObject(context);
    // Register factory and attributes for the Mover component so it can be created via CreateComponent, and loaded / saved
    Mover::RegisterObject(context);
}

void IsometricDemo2D::Setup()
{
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void IsometricDemo2D::Start()
{
    // Execute base class startup
    Sample::Start();

    sample2D_ = new Sample2D{ context_ };

    // Set filename for load/save functions
    sample2D_->demoFilename_ = "Isometric2D";

    // Create the scene content
    CreateScene();

    // Create the UI content
    sample2D_->CreateUIContent("ISOMETRIC 2.5D DEMO", character2D_->remainingLives_, character2D_->remainingCoins_);
    UI* ui{ GetSubsystem<UI>() };
    Button* playButton{ static_cast<Button*>(ui->GetRoot()->GetChild("PlayButton", true)) };
    SubscribeToEvent(playButton, E_RELEASED, DRY_HANDLER(IsometricDemo2D, HandlePlayButton));

    // Hook up to the frame update events
    SubscribeToEvents();
}

void IsometricDemo2D::CreateScene()
{
    scene_ = new Scene{ context_ };
    sample2D_->scene_ = scene_;

    // Create the Octree, DebugRenderer and PhysicsWorld2D components to the scene
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    PhysicsWorld2D* physicsWorld{ scene_->CreateComponent<PhysicsWorld2D>() };
    physicsWorld->SetGravity(Vector2{ 0.0f, 0.0f }); // Neutralize gravity as the character will always be grounded

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera{ cameraNode_->CreateComponent<Camera>() };
    camera->SetOrthographic(true);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    camera->SetOrthoSize(graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(2.0f * Min(graphics->GetWidth() / 1280.0f, graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (2.0) is set for full visibility at 1280x800 resolution)

    // Setup the viewport for displaying the scene
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, camera } };
    Renderer* renderer{ GetSubsystem<Renderer>() };
    renderer->SetViewport(0, viewport);

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Create tile map from tmx file
    TmxFile2D* tmxFile{ cache->GetResource<TmxFile2D>("2D/Tilesets/atrium.tmx") };
    SharedPtr<Node> tileMapNode{ scene_->CreateChild("TileMap") };
    TileMap2D* tileMap{ tileMapNode->CreateComponent<TileMap2D>() };
    tileMap->SetTmxFile(tmxFile);
    const TileMapInfo2D& info{ tileMap->GetInfo() };

    // Create Spriter Imp character (from sample 33_SpriterAnimation)
    Node* spriteNode{ sample2D_->CreateCharacter(info, 0.0f, Vector3{ -5.0f, 11.0f, 0.0f }, 0.15f) };
    character2D_ = spriteNode->CreateComponent<Character2D>(); // Create a logic component to handle character behavior
    // Scale character's speed on the Y axis according to tiles' aspect ratio
    character2D_->moveSpeedScale_ = info.tileHeight_ / info.tileWidth_;
    character2D_->zoom_ = camera->GetZoom();

    // Generate physics collision shapes from the tmx file's objects located in "Physics" (top) layer
    TileMapLayer2D* tileMapLayer{ tileMap->GetLayer(tileMap->GetNumLayers() - 1) };
    sample2D_->CreateCollisionShapesFromTMXObjects(tileMapNode, tileMapLayer, info);

    // Instantiate enemies at each placeholder of "MovingEntities" layer (placeholders are Poly Line objects defining a path from points)
    sample2D_->PopulateMovingEntities(tileMap->GetLayer(tileMap->GetNumLayers() - 2));

    // Instantiate coins to pick at each placeholder of "Coins" layer (placeholders for coins are Rectangle objects)
    TileMapLayer2D* coinsLayer{ tileMap->GetLayer(tileMap->GetNumLayers() - 3) };
    sample2D_->PopulateCoins(coinsLayer);

    // Init coins counters
    character2D_->remainingCoins_ = coinsLayer->GetNumObjects();
    character2D_->maxCoins_ = coinsLayer->GetNumObjects();

    // Check when scene is rendered
    SubscribeToEvent(E_ENDRENDERING, DRY_HANDLER(IsometricDemo2D, HandleSceneRendered));
}

void IsometricDemo2D::HandleCollisionBegin(StringHash eventType, VariantMap& eventData)
{
    // Get colliding node
    Node* hitNode{ static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEA].GetPtr()) };

    if (hitNode->GetName() == "Imp")
        hitNode = static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEB].GetPtr());

    const String nodeName{ hitNode->GetName() };
    Node* character2DNode{ scene_->GetChild("Imp", true) };

    // Handle coins picking
    if (nodeName == "Coin")
    {
        hitNode->Remove();
        character2D_->remainingCoins_ -= 1;
        UI* ui{ GetSubsystem<UI>() };

        if (character2D_->remainingCoins_ == 0)
        {
            Text* instructions{ static_cast<Text*>(ui->GetRoot()->GetChild("Instructions", true)) };
            instructions->SetText("!!! You have all the coins !!!");
        }

        Text* coinsText{ static_cast<Text*>(ui->GetRoot()->GetChild("CoinsText", true)) };
        coinsText->SetText(String(character2D_->remainingCoins_)); // Update coins UI counter
        sample2D_->PlaySoundEffect("Powerup.wav");
    }

    // Handle interactions with enemies
    if (nodeName == "Orc")
    {
        AnimatedSprite2D* animatedSprite{ character2DNode->GetComponent<AnimatedSprite2D>() };
        const float deltaX{ character2DNode->GetPosition().x_ - hitNode->GetPosition().x_ };

        // Orc killed if character is fighting in its direction when the contact occurs
        if (animatedSprite->GetAnimation() == "attack" && (deltaX < 0 == animatedSprite->GetFlipX()))
        {
            static_cast<Mover*>(hitNode->GetComponent<Mover>())->emitTime_ = 1.0f;

            if (!hitNode->GetChild("Emitter", true))
            {
                hitNode->GetComponent("RigidBody2D")->Remove(); // Remove Orc's body
                sample2D_->SpawnEffect(hitNode);
                sample2D_->PlaySoundEffect("BigExplosion.wav");
            }
        }
        // Player killed if not fighting in the direction of the Orc when the contact occurs
        else
        {
            if (!character2DNode->GetChild("Emitter", true))
            {
                character2D_->wounded_ = true;

                if (nodeName == "Orc")
                {
                    Mover* orc{ static_cast<Mover*>(hitNode->GetComponent<Mover>()) };
                    orc->fightTimer_ = 1.0f;
                }

                sample2D_->SpawnEffect(character2DNode);
                sample2D_->PlaySoundEffect("BigExplosion.wav");
            }
        }
    }
}

void IsometricDemo2D::HandleSceneRendered(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(E_ENDRENDERING);
    // Save the scene so we can reload it later
    sample2D_->SaveScene(true);
    // Pause the scene as long as the UI is hiding it
    scene_->SetUpdateEnabled(false);
}

void IsometricDemo2D::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(IsometricDemo2D, HandleUpdate));

    // Subscribe HandlePostUpdate() function for processing post update events
    SubscribeToEvent(E_POSTUPDATE, DRY_HANDLER(IsometricDemo2D, HandlePostUpdate));

    // Subscribe to PostRenderUpdate to draw debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(IsometricDemo2D, HandlePostRenderUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);

    // Subscribe to Box2D contact listeners
    SubscribeToEvent(E_PHYSICSBEGINCONTACT2D, DRY_HANDLER(IsometricDemo2D, HandleCollisionBegin));
}

void IsometricDemo2D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Zoom in/out
    if (cameraNode_)
        sample2D_->Zoom(cameraNode_->GetComponent<Camera>());

    Input* input{ GetSubsystem<Input>() };

    // Toggle debug geometry with 'Z' key
    if (input->GetKeyPress(KEY_Z))
        drawDebug_ = !drawDebug_;

    // Check for loading / saving the scene
    if (input->GetKeyPress(KEY_F5))
        sample2D_->SaveScene(false);

    if (input->GetKeyPress(KEY_F7))
        ReloadScene(false);
}

void IsometricDemo2D::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character2D_)
        return;

    Node* character2DNode{ character2D_->GetNode() };
    cameraNode_->SetPosition(Vector3(character2DNode->GetPosition().x_, character2DNode->GetPosition().y_, -10.0f)); // Camera tracks character
}

void IsometricDemo2D::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (drawDebug_)
    {
        PhysicsWorld2D* physicsWorld{ scene_->GetComponent<PhysicsWorld2D>() };
        Node* tileMapNode{ scene_->GetChild("TileMap", true) };
        TileMap2D* map{ tileMapNode->GetComponent<TileMap2D>() };

        physicsWorld->DrawDebugGeometry();
        map->DrawDebugGeometry(scene_->GetComponent<DebugRenderer>(), false);
    }
}

void IsometricDemo2D::ReloadScene(bool reInit)
{
    String filename{ sample2D_->demoFilename_ };

    if (!reInit)
        filename += "InGame";

    File loadFile{ context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + filename + ".xml", FILE_READ };
    scene_->LoadXML(loadFile);
    // After loading we have to reacquire the weak pointer to the Character2D component, as it has been recreated
    // Simply find the character's scene node by name as there's only one of them
    Node* character2DNode{ scene_->GetChild("Imp", true) };

    if (character2DNode)
        character2D_ = character2DNode->GetComponent<Character2D>();

    // Set what number to use depending whether reload is requested from 'PLAY' button (reInit=true) or 'F7' key (reInit=false)
    int lives{ character2D_->remainingLives_ };
    int coins{ character2D_->remainingCoins_ };

    if (reInit)
    {
        lives = LIVES;
        coins = character2D_->maxCoins_;
    }

    // Update lifes UI
    UI* ui{ GetSubsystem<UI>() };
    Text* lifeText{ static_cast<Text*>(ui->GetRoot()->GetChild("LifeText", true)) };
    lifeText->SetText(String{ lives });

    // Update coins UI
    Text* coinsText{ static_cast<Text*>(ui->GetRoot()->GetChild("CoinsText", true)) };
    coinsText->SetText(String{ coins });
}

void IsometricDemo2D::HandlePlayButton(StringHash eventType, VariantMap& eventData)
{
    // Remove fullscreen UI and unfreeze the scene
    UI* ui{ GetSubsystem<UI>() };

    if (static_cast<Text*>(ui->GetRoot()->GetChild("FullUI", true)))
    {
        ui->GetRoot()->GetChild("FullUI", true)->Remove();
        scene_->SetUpdateEnabled(true);
    }
    else
    {
        // Reload scene
        ReloadScene(true);
    }

    // Hide Instructions and Play/Exit buttons
    Text* instructionText{ static_cast<Text*>(ui->GetRoot()->GetChild("Instructions", true)) };
    instructionText->SetText("");
    Button* exitButton{ static_cast<Button*>(ui->GetRoot()->GetChild("ExitButton", true)) };
    exitButton->SetVisible(false);
    Button* playButton{ static_cast<Button*>(ui->GetRoot()->GetChild("PlayButton", true)) };
    playButton->SetVisible(false);

    // Hide mouse cursor
    Input* input{ GetSubsystem<Input>() };
    input->SetMouseVisible(false);
}
