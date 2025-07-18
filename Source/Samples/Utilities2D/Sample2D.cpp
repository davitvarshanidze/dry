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
#include <Dry/UI/BorderImage.h>
#include <Dry/UI/Button.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/2D/CollisionBox2D.h>
#include <Dry/2D/CollisionChain2D.h>
#include <Dry/2D/CollisionCircle2D.h>
#include <Dry/2D/CollisionPolygon2D.h>
#include <Dry/Core/Context.h>
#include <Dry/Core/CoreEvents.h>
#include <Dry/Engine/Engine.h>
#include <Dry/IO/File.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/UI/Font.h>
#include <Dry/Input/Input.h>
#include <Dry/2D/ParticleEffect2D.h>
#include <Dry/2D/ParticleEmitter2D.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/2D/RigidBody2D.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Audio/Sound.h>
#include <Dry/Audio/SoundSource.h>
#include <Dry/Core/StringUtils.h>
#include <Dry/UI/Text.h>
#include <Dry/Graphics/Texture2D.h>
#include <Dry/2D/TileMap2D.h>
#include <Dry/2D/TileMapLayer2D.h>
#include <Dry/2D/TmxFile2D.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/UI/Window.h>

#include "Utilities2D/Mover.h"
#include "Sample2D.h"


Sample2D::Sample2D(Context* context) :
    Object(context)
{
}

void Sample2D::CreateCollisionShapesFromTMXObjects(Node* tileMapNode, TileMapLayer2D* tileMapLayer, const TileMapInfo2D& info)
{
    // Create rigid body to the root node
    auto* body = tileMapNode->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_STATIC);

    // Generate physics collision shapes and rigid bodies from the tmx file's objects located in "Physics" layer
    for (unsigned i = 0; i < tileMapLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* tileMapObject = tileMapLayer->GetObject(i); // Get physics objects

        // Create collision shape from tmx object
        switch (tileMapObject->GetObjectType())
        {
            case OT_RECTANGLE:
            {
                CreateRectangleShape(tileMapNode, tileMapObject, tileMapObject->GetSize(), info);
            }
            break;

            case OT_ELLIPSE:
            {
                CreateCircleShape(tileMapNode, tileMapObject, tileMapObject->GetSize().x_ / 2, info); // Ellipse is built as a Circle shape as it doesn't exist in Box2D
            }
            break;

            case OT_POLYGON:
            {
                CreatePolygonShape(tileMapNode, tileMapObject);
            }
            break;

            case OT_POLYLINE:
            {
                CreatePolyLineShape(tileMapNode, tileMapObject);
            }
            break;
        }
    }
}

CollisionBox2D* Sample2D::CreateRectangleShape(Node* node, TileMapObject2D* object, const Vector2& size, const TileMapInfo2D& info)
{
    auto* shape = node->CreateComponent<CollisionBox2D>();
    shape->SetSize(size);
    if (info.orientation_ == O_ORTHOGONAL)
        shape->SetCenter(object->GetPosition() + size / 2);
    else
    {
        shape->SetCenter(object->GetPosition() + Vector2(info.tileWidth_ / 2, 0.0f));
        shape->SetAngle(45.0f); // If our tile map is isometric then shape is losange
    }
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

CollisionCircle2D* Sample2D::CreateCircleShape(Node* node, TileMapObject2D* object, float radius, const TileMapInfo2D& info)
{
    auto* shape = node->CreateComponent<CollisionCircle2D>();
    Vector2 size = object->GetSize();
    if (info.orientation_ == O_ORTHOGONAL)
        shape->SetCenter(object->GetPosition() + size / 2);
    else
    {
        shape->SetCenter(object->GetPosition() + Vector2(info.tileWidth_ / 2, 0.0f));
    }

    shape->SetRadius(radius);
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

CollisionPolygon2D* Sample2D::CreatePolygonShape(Node* node, TileMapObject2D* object)
{
    auto* shape = node->CreateComponent<CollisionPolygon2D>();
    int numVertices = object->GetNumPoints();
    shape->SetVertexCount(numVertices);
    for (int i = 0; i < numVertices; ++i)
        shape->SetVertex(i, object->GetPoint(i));
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

CollisionChain2D* Sample2D::CreatePolyLineShape(Node* node, TileMapObject2D* object)
{
    auto* shape = node->CreateComponent<CollisionChain2D>();
    int numVertices = object->GetNumPoints();
    shape->SetVertexCount(numVertices);
    for (int i = 0; i < numVertices; ++i)
        shape->SetVertex(i, object->GetPoint(i));
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

Node* Sample2D::CreateCharacter(const TileMapInfo2D& info, float friction, const Vector3& position, float scale)
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* spriteNode = scene_->CreateChild("Imp");
    spriteNode->SetPosition(position);
    spriteNode->SetScale(scale);
    auto* animatedSprite = spriteNode->CreateComponent<AnimatedSprite2D>();
    // Get scml file and Play "idle" anim
    auto* animationSet = cache->GetResource<AnimationSet2D>("2D/imp/imp.scml");
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("idle");
    animatedSprite->SetLayer(3); // Put character over tile map (which is on layer 0) and over Orcs (which are on layer 2)
    auto* impBody = spriteNode->CreateComponent<RigidBody2D>();
    impBody->SetBodyType(BT_DYNAMIC);
    impBody->SetAllowSleep(false);
    auto* shape = spriteNode->CreateComponent<CollisionCircle2D>();
    shape->SetRadius(1.1f); // Set shape size
    shape->SetFriction(friction); // Set friction
    shape->SetRestitution(0.1f); // Bounce

    return spriteNode;
}

Node* Sample2D::CreateTrigger()
{
    Node* node = scene_->CreateChild(); // Clones will be renamed according to object type
    auto* body = node->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->CreateComponent<CollisionBox2D>(); // Create box shape
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateEnemy()
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* node = scene_->CreateChild("Enemy");
    auto* staticSprite = node->CreateComponent<StaticSprite2D>();
    staticSprite->SetSprite(cache->GetResource<Sprite2D>("2D/Aster.png"));
    auto* body = node->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->CreateComponent<CollisionCircle2D>(); // Create circle shape
    shape->SetRadius(0.25f); // Set radius
    return node;
}

Node* Sample2D::CreateOrc()
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* node = scene_->CreateChild("Orc");
    node->SetScale(scene_->GetChild("Imp", true)->GetScale());
    auto* animatedSprite = node->CreateComponent<AnimatedSprite2D>();
    auto* animationSet = cache->GetResource<AnimationSet2D>("2D/Orc/Orc.scml");
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("run"); // Get scml file and Play "run" anim
    animatedSprite->SetLayer(2); // Make orc always visible
    auto* body = node->CreateComponent<RigidBody2D>();
    auto* shape = node->CreateComponent<CollisionCircle2D>();
    shape->SetRadius(1.3f); // Set shape size
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateCoin()
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* node = scene_->CreateChild("Coin");
    node->SetScale(0.5);
    auto* animatedSprite = node->CreateComponent<AnimatedSprite2D>();
    auto* animationSet = cache->GetResource<AnimationSet2D>("2D/GoldIcon.scml");
    animatedSprite->SetAnimationSet(animationSet); // Get scml file and Play "idle" anim
    animatedSprite->SetAnimation("idle");
    animatedSprite->SetLayer(4);
    auto* body = node->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->CreateComponent<CollisionCircle2D>(); // Create circle shape
    shape->SetRadius(0.32f); // Set radius
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateMovingPlatform()
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* node = scene_->CreateChild("MovingPlatform");
    node->SetScale(Vector3(3.0f, 1.0f, 0.0f));
    auto* staticSprite = node->CreateComponent<StaticSprite2D>();
    staticSprite->SetSprite(cache->GetResource<Sprite2D>("2D/Box.png"));
    auto* body = node->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->CreateComponent<CollisionBox2D>(); // Create box shape
    shape->SetSize(Vector2(0.32f, 0.32f)); // Set box size
    shape->SetFriction(0.8f); // Set friction
    return node;
}

void Sample2D::PopulateMovingEntities(TileMapLayer2D* movingEntitiesLayer)
{
    // Create enemy (will be cloned at each placeholder)
    Node* enemyNode = CreateEnemy();
    Node* orcNode = CreateOrc();
    Node* platformNode = CreateMovingPlatform();

    // Instantiate enemies and moving platforms at each placeholder (placeholders are Poly Line objects defining a path from points)
    for (unsigned i=0; i < movingEntitiesLayer->GetNumObjects(); ++i)
    {
        // Get placeholder object
        TileMapObject2D* movingObject = movingEntitiesLayer->GetObject(i); // Get placeholder object
        if (movingObject->GetObjectType() == OT_POLYLINE)
        {
            // Clone the enemy and position it at placeholder point
            Node* movingClone;
            Vector2 offset = Vector2(0.0f, 0.0f);
            if (movingObject->GetType() == "Enemy")
            {
                movingClone = enemyNode->Clone();
                offset = Vector2(0.0f, -0.32f);
            }
            else if (movingObject->GetType() == "Orc")
                movingClone = orcNode->Clone();
            else if (movingObject->GetType() == "MovingPlatform")
                movingClone = platformNode->Clone();
            else
                continue;
            movingClone->SetPosition2D(movingObject->GetPoint(0) + offset);

            // Create script object that handles entity translation along its path
            auto* mover = movingClone->CreateComponent<Mover>();

            // Set path from points
            PODVector<Vector2> path = CreatePathFromPoints(movingObject, offset);
            mover->path_ = path;

            // Override default speed
            if (movingObject->HasProperty("Speed"))
                mover->speed_ = ToFloat(movingObject->GetProperty("Speed"));
        }
    }

    // Remove nodes used for cloning purpose
    enemyNode->Remove();
    orcNode->Remove();
    platformNode->Remove();
}

void Sample2D::PopulateCoins(TileMapLayer2D* coinsLayer)
{
    // Create coin (will be cloned at each placeholder)
    Node* coinNode = CreateCoin();

    // Instantiate coins to pick at each placeholder
    for (unsigned i=0; i < coinsLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* coinObject = coinsLayer->GetObject(i); // Get placeholder object
        Node* coinClone = coinNode->Clone();
        coinClone->SetPosition2D(coinObject->GetPosition() + coinObject->GetSize() / 2 + Vector2(0.0f, 0.16f));

    }

    // Remove node used for cloning purpose
    coinNode->Remove();
}

void Sample2D::PopulateTriggers(TileMapLayer2D* triggersLayer)
{
    // Create trigger node (will be cloned at each placeholder)
    Node* triggerNode = CreateTrigger();

    // Instantiate triggers at each placeholder (Rectangle objects)
    for (unsigned i=0; i < triggersLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* triggerObject = triggersLayer->GetObject(i); // Get placeholder object
        if (triggerObject->GetObjectType() == OT_RECTANGLE)
        {
            Node* triggerClone = triggerNode->Clone();
            triggerClone->SetName(triggerObject->GetType());
            auto* shape = triggerClone->GetComponent<CollisionBox2D>();
            shape->SetSize(triggerObject->GetSize());
            triggerClone->SetPosition2D(triggerObject->GetPosition() + triggerObject->GetSize() / 2);
        }
    }
}

float Sample2D::Zoom(Camera* camera)
{
    auto* input = GetSubsystem<Input>();
    float zoom_ = camera->GetZoom();

    if (input->GetMouseMoveWheel() != 0)
    {
        zoom_ = Clamp(zoom_ + input->GetMouseMoveWheel() * 0.1f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        zoom_ = Clamp(zoom_ * 1.01f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        zoom_ = Clamp(zoom_ * 0.99f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    return zoom_;
}

PODVector<Vector2> Sample2D::CreatePathFromPoints(TileMapObject2D* object, const Vector2& offset)
{
    PODVector<Vector2> path;
    for (unsigned i=0; i < object->GetNumPoints(); ++i)
        path.Push(object->GetPoint(i) + offset);
    return path;
}

void Sample2D::CreateUIContent(const String& demoTitle, int remainingLifes, int remainingCoins)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Set the default UI style and font
    ui->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    // We create in-game UIs (coins and lifes) first so that they are hidden by the fullscreen UI (we could also temporary hide them using SetVisible)

    // Create the UI for displaying the remaining coins
    auto* coinsUI = ui->GetRoot()->CreateChild<BorderImage>("Coins");
    coinsUI->SetTexture(cache->GetResource<Texture2D>("2D/GoldIcon.png"));
    coinsUI->SetSize(50, 50);
    coinsUI->SetImageRect(IntRect(0, 64, 60, 128));
    coinsUI->SetAlignment(HA_LEFT, VA_TOP);
    coinsUI->SetPosition(5, 5);
    auto* coinsText = coinsUI->CreateChild<Text>("CoinsText");
    coinsText->SetAlignment(HA_CENTER, VA_CENTER);
    coinsText->SetFont(font, 24);
    coinsText->SetTextEffect(TE_SHADOW);
    coinsText->SetText(String(remainingCoins));

    // Create the UI for displaying the remaining lifes
    auto* lifeUI = ui->GetRoot()->CreateChild<BorderImage>("Life");
    lifeUI->SetTexture(cache->GetResource<Texture2D>("2D/imp/imp_all.png"));
    lifeUI->SetSize(70, 80);
    lifeUI->SetAlignment(HA_RIGHT, VA_TOP);
    lifeUI->SetPosition(-5, 5);
    auto* lifeText = lifeUI->CreateChild<Text>("LifeText");
    lifeText->SetAlignment(HA_CENTER, VA_CENTER);
    lifeText->SetFont(font, 24);
    lifeText->SetTextEffect(TE_SHADOW);
    lifeText->SetText(String(remainingLifes));

    // Create the fullscreen UI for start/end
    auto* fullUI = ui->GetRoot()->CreateChild<Window>("FullUI");
    fullUI->SetStyleAuto();
    fullUI->SetSize(ui->GetRoot()->GetWidth(), ui->GetRoot()->GetHeight());
    fullUI->SetEnabled(false); // Do not react to input, only the 'Exit' and 'Play' buttons will

    // Create the title
    auto* title = fullUI->CreateChild<BorderImage>("Title");
    title->SetMinSize(fullUI->GetWidth(), 50);
    title->SetTexture(cache->GetResource<Texture2D>("Textures/HeightMap.png"));
    title->SetFullImageRect();
    title->SetAlignment(HA_CENTER, VA_TOP);
    auto* titleText = title->CreateChild<Text>("TitleText");
    titleText->SetAlignment(HA_CENTER, VA_CENTER);
    titleText->SetFont(font, 24);
    titleText->SetText(demoTitle);

    // Create the image
    auto* spriteUI = fullUI->CreateChild<BorderImage>("Sprite");
    spriteUI->SetTexture(cache->GetResource<Texture2D>("2D/imp/imp_all.png"));
    spriteUI->SetSize(238, 271);
    spriteUI->SetAlignment(HA_CENTER, VA_CENTER);
    spriteUI->SetPosition(0, - ui->GetRoot()->GetHeight() / 4);

    // Create the 'EXIT' button
    auto* exitButton = ui->GetRoot()->CreateChild<Button>("ExitButton");
    exitButton->SetStyleAuto();
    exitButton->SetFocusMode(FM_RESETFOCUS);
    exitButton->SetSize(100, 50);
    exitButton->SetAlignment(HA_CENTER, VA_CENTER);
    exitButton->SetPosition(-100, 0);
    auto* exitText = exitButton->CreateChild<Text>("ExitText");
    exitText->SetAlignment(HA_CENTER, VA_CENTER);
    exitText->SetFont(font, 24);
    exitText->SetText("EXIT");
    SubscribeToEvent(exitButton, E_RELEASED, DRY_HANDLER(Sample2D, HandleExitButton));

    // Create the 'PLAY' button
    auto* playButton = ui->GetRoot()->CreateChild<Button>("PlayButton");
    playButton->SetStyleAuto();
    playButton->SetFocusMode(FM_RESETFOCUS);
    playButton->SetSize(100, 50);
    playButton->SetAlignment(HA_CENTER, VA_CENTER);
    playButton->SetPosition(100, 0);
    auto* playText = playButton->CreateChild<Text>("PlayText");
    playText->SetAlignment(HA_CENTER, VA_CENTER);
    playText->SetFont(font, 24);
    playText->SetText("PLAY");
//  SubscribeToEvent(playButton, E_RELEASED, HANDLER(2DPlatformer, HandlePlayButton));

    // Create the instructions
    auto* instructionText = ui->GetRoot()->CreateChild<Text>("Instructions");
    instructionText->SetText("Use WASD keys or Arrows to move\nPageUp/PageDown/MouseWheel to zoom\nF5/F7 to save/reload scene\n'Z' to toggle debug geometry\nSpace to fight");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER); // Center rows in relation to each other
    instructionText->SetAlignment(HA_CENTER, VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);

    // Show mouse cursor
    auto* input = GetSubsystem<Input>();
    input->SetMouseVisible(true);
}

void Sample2D::HandleExitButton(StringHash eventType, VariantMap& eventData)
{
    auto* engine = GetSubsystem<Engine>();
    engine->Exit();
}

void Sample2D::SaveScene(bool initial)
{
    String filename = demoFilename_;
    if (!initial)
        filename += "InGame";
    File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + filename + ".xml", FILE_WRITE);
    scene_->SaveXML(saveFile);
}

void Sample2D::CreateBackgroundSprite(const TileMapInfo2D& info, float scale, const String& texture, bool animate)
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* node = scene_->CreateChild("Background");
    node->SetPosition(Vector3(info.GetMapWidth(), info.GetMapHeight(), 0) / 2);
    node->SetScale(scale);
    auto* sprite = node->CreateComponent<StaticSprite2D>();
    sprite->SetSprite(cache->GetResource<Sprite2D>(texture));
    SetRandomSeed(Time::GetSystemTime()); // Randomize from system clock
    sprite->SetColor(Color(Random(0.0f, 1.0f), Random(0.0f, 1.0f), Random(0.0f, 1.0f), 1.0f));
    sprite->SetLayer(-99);

    // Create rotation animation
    if (animate)
    {
        SharedPtr<ValueAnimation> animation(new ValueAnimation(context_));
        animation->SetKeyFrame(0, Variant(Quaternion(0.0f, 0.0f, 0.0f)));
        animation->SetKeyFrame(1, Variant(Quaternion(0.0f, 0.0f, 180.0f)));
        animation->SetKeyFrame(2, Variant(Quaternion(0.0f, 0.0f, 0.0f)));
        node->SetAttributeAnimation("Rotation", animation, WM_LOOP, 0.05f);
    }
}

void Sample2D::SpawnEffect(Node* node)
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* particleNode = node->CreateChild("Emitter");
    particleNode->SetScale(0.5f / node->GetScale().x_);
    auto* particleEmitter = particleNode->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetLayer(2);
    particleEmitter->SetEffect(cache->GetResource<ParticleEffect2D>("2D/sun.pex"));
}

void Sample2D::PlaySoundEffect(const String& soundName)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* source = scene_->CreateComponent<SoundSource>();
    auto* sound = cache->GetResource<Sound>("Sounds/" + soundName);
    if (sound != nullptr) {
        source->SetAutoRemoveMode(REMOVE_COMPONENT);
        source->Play(sound);
    }
}
