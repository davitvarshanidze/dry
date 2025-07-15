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

#include <Dry/Container/Vector.h>
#include <Dry/Core/CoreEvents.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/SceneEvents.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/Text3D.h>
#include <Dry/2D/CollisionBox2D.h>
#include <Dry/2D/CollisionCircle2D.h>
#include <Dry/2D/CollisionEdge2D.h>
#include <Dry/2D/CollisionPolygon2D.h>
#include <Dry/2D/ConstraintDistance2D.h>
#include <Dry/2D/ConstraintFriction2D.h>
#include <Dry/2D/ConstraintGear2D.h>
#include <Dry/2D/ConstraintMotor2D.h>
#include <Dry/2D/ConstraintMouse2D.h>
#include <Dry/2D/ConstraintPrismatic2D.h>
#include <Dry/2D/ConstraintPulley2D.h>
#include <Dry/2D/ConstraintRevolute2D.h>
#include <Dry/2D/ConstraintRope2D.h>
#include <Dry/2D/ConstraintWeld2D.h>
#include <Dry/2D/ConstraintWheel2D.h>
#include <Dry/2D/Drawable2D.h>
#include <Dry/2D/PhysicsWorld2D.h>
#include <Dry/2D/RigidBody2D.h>
#include <Dry/2D/Sprite2D.h>
#include <Dry/2D/StaticSprite2D.h>

#include "Constraints2D.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(Constraints2D)

Node* pickedNode;
RigidBody2D* dummyBody;

Constraints2D::Constraints2D(Context* context): Sample(context)
{
}

void Constraints2D::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the UI content
    CreateInstructions("Use WASD keys to move, QE to zoom and mouse to interact\n"
                       "Space to toggle debug geometry and joints - F5 to save the scene.");

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Constraints2D::CreateScene()
{
    scene_ = new Scene{ context_ };
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    PhysicsWorld2D* physicsWorld{ scene_->CreateComponent<PhysicsWorld2D>() }; // Create 2D physics world component
    physicsWorld->SetDrawJoint(true); // Display the joints (Note that DrawDebugGeometry() must be set to true to acually draw the joints)
    drawDebug_ = true; // Set DrawDebugGeometry() to true

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition({ 0.0f, 0.0f, 0.0f }); // Note that Z setting is discarded; use camera.zoom instead (see MoveCamera() below for example)

    camera_ = cameraNode_->CreateComponent<Camera>();
    camera_->SetOrthographic(true);

    Graphics* graphics{ GetSubsystem<Graphics>() };
    camera_->SetOrthoSize(graphics->GetHeight() * PIXEL_SIZE);
    camera_->SetZoom(Min(graphics->GetWidth() / 1280.0f, graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, camera_ } };
    Renderer* renderer{ GetSubsystem<Renderer>() };
    renderer->SetViewport(0, viewport);

    Zone* zone{ renderer->GetDefaultZone() };
    zone->SetFogColor({ 0.1f, 0.1f, 0.1f }); // Set background color for the scene

    // Create 4x3 grid
    for (unsigned i{ 0 }; i < 5; ++i)
    {
        Node* edgeNode{ scene_->CreateChild("VerticalEdge") };
        RigidBody2D* edgeBody{ edgeNode->CreateComponent<RigidBody2D>() };

        if (!dummyBody)
            dummyBody = edgeBody; // Mark first edge as dummy body (used by mouse pick)

        CollisionEdge2D* edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
        edgeShape->SetVertices(Vector2{ i*2.5f -5.0f, -3.0f }, Vector2{ i*2.5f -5.0f, 3.0f });
        edgeShape->SetFriction(0.5f); // Set friction
    }

    for (unsigned j{ 0 }; j < 4; ++j)
    {
        Node* edgeNode{ scene_->CreateChild("HorizontalEdge") };
        /*RigidBody2D* edgeBody = */edgeNode->CreateComponent<RigidBody2D>();
        CollisionEdge2D* edgeShape{ edgeNode->CreateComponent<CollisionEdge2D>() };
        edgeShape->SetVertices({ -5.0f, j * 2.0f -3.0f }, { 5.0f, j * 2.0f -3.0f });
        edgeShape->SetFriction(0.5f); // Set friction
    }

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Create a box (will be cloned later)
    Node* box{ scene_->CreateChild("Box") };
    box->SetPosition({ 0.8f, -2.0f, 0.0f });
    StaticSprite2D* boxSprite{ box->CreateComponent<StaticSprite2D>() };
    boxSprite->SetSprite(cache->GetResource<Sprite2D>("2D/Box.png"));
    RigidBody2D* boxBody{ box->CreateComponent<RigidBody2D>() };
    boxBody->SetBodyType(BT_DYNAMIC);
    boxBody->SetLinearDamping(0.0f);
    boxBody->SetAngularDamping(0.0f);
    CollisionBox2D* shape{ box->CreateComponent<CollisionBox2D>() }; // Create box shape
    shape->SetSize(Vector2{ 0.32f, 0.32f }); // Set size
    shape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    shape->SetFriction(0.5f); // Set friction
    shape->SetRestitution(0.1f); // Set restitution (slight bounce)

    // Create a ball (will be cloned later)
    Node* ball{ scene_->CreateChild("Ball") };
    ball->SetPosition({ 1.8f, -2.0f, 0.0f });
    StaticSprite2D* ballSprite{ ball->CreateComponent<StaticSprite2D>() };
    ballSprite->SetSprite(cache->GetResource<Sprite2D>("2D/Ball.png"));
    RigidBody2D* ballBody{ ball->CreateComponent<RigidBody2D>() };
    ballBody->SetBodyType(BT_DYNAMIC);
    ballBody->SetLinearDamping(0.0f);
    ballBody->SetAngularDamping(0.0f);
    CollisionCircle2D* ballShape{ ball->CreateComponent<CollisionCircle2D>() }; // Create circle shape
    ballShape->SetRadius(0.16f); // Set radius
    ballShape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    ballShape->SetFriction(0.5f); // Set friction
    ballShape->SetRestitution(0.6f); // Set restitution: make it bounce

    // Create a polygon
    Node* polygon{ scene_->CreateChild("Polygon") };
    polygon->SetPosition({ 1.6f, -2.0f, 0.0f });
    polygon->SetScale(0.7f);
    StaticSprite2D* polygonSprite{ polygon->CreateComponent<StaticSprite2D>() };
    polygonSprite->SetSprite(cache->GetResource<Sprite2D>("2D/Aster.png"));
    RigidBody2D* polygonBody{ polygon->CreateComponent<RigidBody2D>() };
    polygonBody->SetBodyType(BT_DYNAMIC);
    CollisionPolygon2D* polygonShape{ polygon->CreateComponent<CollisionPolygon2D>() };
    // TODO: create from PODVector<Vector2> using SetVertices()
    polygonShape->SetVertexCount(6); // Set number of vertices (mandatory when using SetVertex())
    polygonShape->SetVertex(0, Vector2{ -0.8f, -0.3f });
    polygonShape->SetVertex(1, Vector2{  0.5f, -0.8f });
    polygonShape->SetVertex(2, Vector2{  0.8f, -0.3f });
    polygonShape->SetVertex(3, Vector2{  0.8f,  0.5f });
    polygonShape->SetVertex(4, Vector2{  0.5f,  0.9f });
    polygonShape->SetVertex(5, Vector2{ -0.5f,  0.7f });
    polygonShape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    polygonShape->SetFriction(0.3f); // Set friction
    polygonShape->SetRestitution(0.0f); // Set restitution (no bounce)

    // Create a ConstraintDistance2D
    CreateFlag("ConstraintDistance2D", -4.97f, 3.0f); // Display Text3D flag
    Node* boxDistanceNode{ box->Clone() };
    Node* ballDistanceNode{ ball->Clone() };
    RigidBody2D* ballDistanceBody{ ballDistanceNode->GetComponent<RigidBody2D>() };
    boxDistanceNode->SetPosition({ -4.5f, 2.0f, 0.0f });
    ballDistanceNode->SetPosition({ -3.0f, 2.0f, 0.0f });

    ConstraintDistance2D* constraintDistance{ boxDistanceNode->CreateComponent<ConstraintDistance2D>() }; // Apply ConstraintDistance2D to box
    constraintDistance->SetOtherBody(ballDistanceBody); // Constrain ball to box
    constraintDistance->SetOwnerBodyAnchor(boxDistanceNode->GetPosition2D());
    constraintDistance->SetOtherBodyAnchor(ballDistanceNode->GetPosition2D());
    // Make the constraint soft (comment to make it rigid, which is its basic behavior)
    constraintDistance->SetFrequencyHz(4.0f);
    constraintDistance->SetDampingRatio(0.5f);

    // Create a ConstraintFriction2D ********** Not functional. From Box2d samples it seems that 2 anchors are required, 2D only provides 1, needs investigation ***********
    CreateFlag("ConstraintFriction2D", 0.03f, 1.0f); // Display Text3D flag
    Node* boxFrictionNode{ box->Clone() };
    Node* ballFrictionNode{ ball->Clone() };
    boxFrictionNode->SetPosition({ 0.5f, 0.0f, 0.0f });
    ballFrictionNode->SetPosition({ 1.5f, 0.0f, 0.0f });

    ConstraintFriction2D* constraintFriction{ boxFrictionNode->CreateComponent<ConstraintFriction2D>() }; // Apply ConstraintDistance2D to box
    constraintFriction->SetOtherBody(ballFrictionNode->GetComponent<RigidBody2D>()); // Constraint ball to box
    //constraintFriction->SetOwnerBodyAnchor(boxNode->GetPosition2D());
    //constraintFriction->SetOtherBodyAnchor(ballNode->GetPosition2D());
    //constraintFriction->SetMaxForce(10.0f); // ballBody.mass * gravity
    //constraintDistance->SetMaxTorque(10.0f); // ballBody.mass * radius * gravity

    // Create a ConstraintGear2D
    CreateFlag("ConstraintGear2D", -4.97f, -1.0f); // Display Text3D flag
    Node* baseNode{ box->Clone() };
    RigidBody2D* tempBody{ baseNode->GetComponent<RigidBody2D>() }; // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    baseNode->SetPosition({ -3.7f, -2.5f, 0.0f });
    Node* ball1Node{ ball->Clone() };
    ball1Node->SetPosition({ -4.5f, -2.0f, 0.0f });
    RigidBody2D* ball1Body{ ball1Node->GetComponent<RigidBody2D>() };
    Node* ball2Node{ ball->Clone() };
    ball2Node->SetPosition({ -3.0f, -2.0f, 0.0f });
    RigidBody2D* ball2Body{ ball2Node->GetComponent<RigidBody2D>() };

    ConstraintRevolute2D* gear1{ baseNode->CreateComponent<ConstraintRevolute2D>() }; // Apply constraint to baseBox
    gear1->SetOtherBody(ball1Body); // Constrain ball1 to baseBox
    gear1->SetAnchor(ball1Node->GetPosition2D());
    ConstraintRevolute2D* gear2{ baseNode->CreateComponent<ConstraintRevolute2D>() }; // Apply constraint to baseBox
    gear2->SetOtherBody(ball2Body); // Constrain ball2 to baseBox
    gear2->SetAnchor(ball2Node->GetPosition2D());

    ConstraintGear2D* constraintGear{ ball1Node->CreateComponent<ConstraintGear2D>() }; // Apply constraint to ball1
    constraintGear->SetOtherBody(ball2Body); // Constrain ball2 to ball1
    constraintGear->SetOwnerConstraint(gear1);
    constraintGear->SetOtherConstraint(gear2);
    constraintGear->SetRatio(1.0f);

    ball1Body->ApplyAngularImpulse(0.015f, true); // Animate

    // Create a vehicle from a compound of 2 ConstraintWheel2Ds
    CreateFlag("ConstraintWheel2Ds compound", -2.45f, -1.0f); // Display Text3D flag
    Node* car{ box->Clone() };
    car->SetScale({ 4.0f, 1.0f, 0.0f });
    car->SetPosition({ -1.2f, -2.3f, 0.0f });
    StaticSprite2D* tempSprite{ car->GetComponent<StaticSprite2D>() }; // Get car Sprite in order to draw it on top
    tempSprite->SetOrderInLayer(0); // Draw car on top of the wheels (set to -1 to draw below)
    Node* ball1WheelNode{ ball->Clone() };
    ball1WheelNode->SetPosition({ -1.6f, -2.5f, 0.0f });
    Node* ball2WheelNode{ ball->Clone() };
    ball2WheelNode->SetPosition({ -0.8f, -2.5f, 0.0f });

    ConstraintWheel2D* wheel1{ car->CreateComponent<ConstraintWheel2D>() };
    wheel1->SetOtherBody(ball1WheelNode->GetComponent<RigidBody2D>());
    wheel1->SetAnchor(ball1WheelNode->GetPosition2D());
    wheel1->SetAxis(Vector2{ 0.0f, 1.0f });
    wheel1->SetMaxMotorTorque(20.0f);
    wheel1->SetFrequencyHz(4.0f);
    wheel1->SetDampingRatio(0.4f);

    ConstraintWheel2D* wheel2{ car->CreateComponent<ConstraintWheel2D>() };
    wheel2->SetOtherBody(ball2WheelNode->GetComponent<RigidBody2D>());
    wheel2->SetAnchor(ball2WheelNode->GetPosition2D());
    wheel2->SetAxis(Vector2{ 0.0f, 1.0f });
    wheel2->SetMaxMotorTorque(10.0f);
    wheel2->SetFrequencyHz(4.0f);
    wheel2->SetDampingRatio(0.4f);

    // ConstraintMotor2D
    CreateFlag("ConstraintMotor2D", 2.53f, -1.0f); // Display Text3D flag
    Node* boxMotorNode{ box->Clone() };
    tempBody = boxMotorNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballMotorNode = ball->Clone();
    boxMotorNode->SetPosition({ 3.8f, -2.1f, 0.0f });
    ballMotorNode->SetPosition({ 3.8f, -1.5f, 0.0f });

    ConstraintMotor2D* constraintMotor{ boxMotorNode->CreateComponent<ConstraintMotor2D>() };
    constraintMotor->SetOtherBody(ballMotorNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintMotor->SetLinearOffset(Vector2{ 0.0f, 0.8f }); // Set ballNode position relative to boxNode position = (0,0)
    constraintMotor->SetAngularOffset(0.1f);
    constraintMotor->SetMaxForce(5.0f);
    constraintMotor->SetMaxTorque(10.0f);
    constraintMotor->SetCorrectionFactor(1.0f);
    constraintMotor->SetCollideConnected(true); // doesn't work

    // ConstraintMouse2D is demonstrated in HandleMouseButtonDown() function. It is used to "grasp" the sprites with the mouse.
    CreateFlag("ConstraintMouse2D", 0.03f, -1.0f); // Display Text3D flag

    // Create a ConstraintPrismatic2D
    CreateFlag("ConstraintPrismatic2D", 2.53f, 3.0f); // Display Text3D flag
    Node* boxPrismaticNode{ box->Clone() };
    tempBody = boxPrismaticNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballPrismaticNode{ ball->Clone() };
    boxPrismaticNode->SetPosition({ 3.3f, 2.5f, 0.0f });
    ballPrismaticNode->SetPosition({ 4.3f, 2.0f, 0.0f });

    ConstraintPrismatic2D* constraintPrismatic{ boxPrismaticNode->CreateComponent<ConstraintPrismatic2D>() };
    constraintPrismatic->SetOtherBody(ballPrismaticNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintPrismatic->SetAxis(Vector2{ 1.0f, 1.0f }); // Slide from [0,0] to [1,1]
    constraintPrismatic->SetAnchor(Vector2{ 4.0f, 2.0f });
    constraintPrismatic->SetLowerTranslation(-1.0f);
    constraintPrismatic->SetUpperTranslation(0.5f);
    constraintPrismatic->SetEnableLimit(true);
    constraintPrismatic->SetMaxMotorForce(1.0f);
    constraintPrismatic->SetMotorSpeed(0.0f);

    // ConstraintPulley2D
    CreateFlag("ConstraintPulley2D", 0.03f, 3.0f); // Display Text3D flag
    Node* boxPulleyNode{ box->Clone() };
    Node* ballPulleyNode{ ball->Clone() };
    boxPulleyNode->SetPosition({ 0.5f, 2.0f, 0.0f });
    ballPulleyNode->SetPosition({ 2.0f, 2.0f, 0.0f });

    ConstraintPulley2D* constraintPulley{ boxPulleyNode->CreateComponent<ConstraintPulley2D>() }; // Apply constraint to box
    constraintPulley->SetOtherBody(ballPulleyNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintPulley->SetOwnerBodyAnchor(boxPulleyNode->GetPosition2D());
    constraintPulley->SetOtherBodyAnchor(ballPulleyNode->GetPosition2D());
    constraintPulley->SetOwnerBodyGroundAnchor(boxPulleyNode->GetPosition2D() + Vector2{ 0.0f, 1.0f });
    constraintPulley->SetOtherBodyGroundAnchor(ballPulleyNode->GetPosition2D() + Vector2{ 0.0f, 1.0f });
    constraintPulley->SetRatio(1.0); // Weight ratio between ownerBody and otherBody

    // Create a ConstraintRevolute2D
    CreateFlag("ConstraintRevolute2D", -2.45f, 3.0f); // Display Text3D flag
    Node* boxRevoluteNode{ box->Clone() };
    tempBody = boxRevoluteNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballRevoluteNode{ ball->Clone() };
    boxRevoluteNode->SetPosition({ -2.0f, 1.5f, 0.0f });
    ballRevoluteNode->SetPosition({ -1.0f, 2.0f, 0.0f });

    ConstraintRevolute2D* constraintRevolute{ boxRevoluteNode->CreateComponent<ConstraintRevolute2D>() }; // Apply constraint to box
    constraintRevolute->SetOtherBody(ballRevoluteNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintRevolute->SetAnchor(Vector2{ -1.0f, 1.5f });
    constraintRevolute->SetLowerAngle(-1.0f); // In radians
    constraintRevolute->SetUpperAngle(0.5f); // In radians
    constraintRevolute->SetEnableLimit(true);
    constraintRevolute->SetMaxMotorTorque(10.0f);
    constraintRevolute->SetMotorSpeed(0.0f);
    constraintRevolute->SetEnableMotor(true);

    // Create a ConstraintRope2D
    CreateFlag("ConstraintRope2D", -4.97f, 1.0f); // Display Text3D flag
    Node* boxRopeNode{ box->Clone() };
    tempBody = boxRopeNode->GetComponent<RigidBody2D>();
    tempBody->SetBodyType(BT_STATIC);
    Node* ballRopeNode{ ball->Clone() };
    boxRopeNode->SetPosition({ -3.7f, 0.7f, 0.0f });
    ballRopeNode->SetPosition({ -4.5f, 0.0f, 0.0f });

    ConstraintRope2D* constraintRope{ boxRopeNode->CreateComponent<ConstraintRope2D>() };
    constraintRope->SetOtherBody(ballRopeNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintRope->SetOwnerBodyAnchor(Vector2{ 0.0f, -0.5f }); // Offset from box (OwnerBody) : the rope is rigid from OwnerBody center to this ownerBodyAnchor
    constraintRope->SetMaxLength(0.9f); // Rope length
    constraintRope->SetCollideConnected(true);

    // Create a ConstraintWeld2D
    CreateFlag("ConstraintWeld2D", -2.45f, 1.0f); // Display Text3D flag
    Node* boxWeldNode{ box->Clone() };
    Node* ballWeldNode{ ball->Clone() };
    boxWeldNode->SetPosition({ -0.5f, 0.0f, 0.0f });
    ballWeldNode->SetPosition({ -2.0f, 0.0f, 0.0f });

    ConstraintWeld2D* constraintWeld{ boxWeldNode->CreateComponent<ConstraintWeld2D>() };
    constraintWeld->SetOtherBody(ballWeldNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintWeld->SetAnchor(boxWeldNode->GetPosition2D());
    constraintWeld->SetFrequencyHz(4.0f);
    constraintWeld->SetDampingRatio(0.5f);

    // Create a ConstraintWheel2D
    CreateFlag("ConstraintWheel2D",  2.53f, 1.0f); // Display Text3D flag
    Node* boxWheelNode{ box->Clone() };
    Node* ballWheelNode{ ball->Clone() };
    boxWheelNode->SetPosition({ 3.8f, 0.0f, 0.0f });
    ballWheelNode->SetPosition({ 3.8f, 0.9f, 0.0f });

    ConstraintWheel2D* constraintWheel{ boxWheelNode->CreateComponent<ConstraintWheel2D>() };
    constraintWheel->SetOtherBody(ballWheelNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintWheel->SetAnchor(ballWheelNode->GetPosition2D());
    constraintWheel->SetAxis(Vector2{ 0.0f, 1.0f });
    constraintWheel->SetEnableMotor(true);
    constraintWheel->SetMaxMotorTorque(1.0f);
    constraintWheel->SetMotorSpeed(0.0f);
    constraintWheel->SetFrequencyHz(4.0f);
    constraintWheel->SetDampingRatio(0.5f);
    constraintWheel->SetCollideConnected(true); // doesn't work
}

void Constraints2D::CreateFlag(const String& text, float x, float y) // Used to create Tex3D flags
{
    Node* flagNode{ scene_->CreateChild("Flag") };
    flagNode->SetPosition({ x, y, 0.0f });
    Text3D* flag3D{ flagNode->CreateComponent<Text3D>() }; // We use Text3D in order to make the text affected by zoom (so that it sticks to 2D)
    flag3D->SetText(text);
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    flag3D->SetFont(cache->GetResource<Font>("Fonts/Philosopher.ttf"), 15);
}

void Constraints2D::MoveCamera(float timeStep)
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
        camera_->SetZoom(camera_->GetZoom() * 1.01f);
    if (input->GetKeyDown(KEY_E))
        camera_->SetZoom(camera_->GetZoom() * 0.99f);
}

void Constraints2D::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(Constraints2D, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DRY_HANDLER(Constraints2D, HandlePostRenderUpdate));

    // Subscribe to mouse click
    SubscribeToEvent(E_MOUSEBUTTONDOWN, DRY_HANDLER(Constraints2D, HandleMouseButtonDown));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);

    if (touchEnabled_)
        SubscribeToEvent(E_TOUCHBEGIN, DRY_HANDLER(Constraints2D, HandleTouchBegin3));
}

void Constraints2D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    Input* input{ GetSubsystem<Input>() };

    // Toggle physics debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;

    // Save scene
    if (input->GetKeyPress(KEY_F5))
    {
        File saveFile{ context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Constraints.xml", FILE_WRITE };
        scene_->SaveXML(saveFile);
    }
}

void Constraints2D::HandlePostRenderUpdate(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    PhysicsWorld2D* physicsWorld{ scene_->GetComponent<PhysicsWorld2D>() };

    if (drawDebug_)
        physicsWorld->DrawDebugGeometry();
}

void Constraints2D::HandleMouseButtonDown(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    Input* input{ GetSubsystem<Input>() };
    PhysicsWorld2D* physicsWorld{ scene_->GetComponent<PhysicsWorld2D>() };
    RigidBody2D* rigidBody{ physicsWorld->GetRigidBodyAtScreenPoint(input->GetMousePosition()) }; // Raycast for RigidBody2Ds to pick

    if (rigidBody)
    {
        pickedNode = rigidBody->GetNode();
        //log.Info(pickedNode.name);
        StaticSprite2D* staticSprite{ pickedNode->GetComponent<StaticSprite2D>() };
        staticSprite->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        ConstraintMouse2D* constraintMouse{ pickedNode->CreateComponent<ConstraintMouse2D>() };
        constraintMouse->SetTarget(GetMousePositionXY());
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody);  // Use dummy body instead of rigidBody. It's better to create a dummy body automatically in ConstraintMouse2D
    }

    SubscribeToEvent(E_MOUSEMOVE, DRY_HANDLER(Constraints2D, HandleMouseMove));
    SubscribeToEvent(E_MOUSEBUTTONUP, DRY_HANDLER(Constraints2D, HandleMouseButtonUp));
}

void Constraints2D::HandleMouseButtonUp(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (pickedNode)
    {
        StaticSprite2D* staticSprite{ pickedNode->GetComponent<StaticSprite2D>() };
        staticSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // Restore picked sprite color

        pickedNode->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode = nullptr;
    }
    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONUP);
}

void Constraints2D::HandleMouseMove(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (pickedNode)
    {
        ConstraintMouse2D* constraintMouse{ pickedNode->GetComponent<ConstraintMouse2D>() };
        constraintMouse->SetTarget(GetMousePositionXY());
    }
}

void Constraints2D::HandleTouchBegin3(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace TouchBegin;

    Graphics* graphics{ GetSubsystem<Graphics>() };
    PhysicsWorld2D* physicsWorld{ scene_->GetComponent<PhysicsWorld2D>() };
    RigidBody2D* rigidBody{ physicsWorld->GetRigidBodyAtScreenPoint(
                            IntVector2{ eventData[P_X].GetInt(), eventData[P_Y].GetInt() }) }; // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        pickedNode = rigidBody->GetNode();

        StaticSprite2D* staticSprite{ pickedNode->GetComponent<StaticSprite2D>() };
        staticSprite->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // Temporary modify color of the picked sprite

        RigidBody2D* rigidBody{ pickedNode->GetComponent<RigidBody2D>() };

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with touch
        ConstraintMouse2D* constraintMouse{ pickedNode->CreateComponent<ConstraintMouse2D>() };
        IntVector2 screenPoint{ eventData[P_X].GetInt(), eventData[P_Y].GetInt() };
        Vector2 pos{ VectorTo2D(camera_->ScreenToWorldPos(graphics->NormalizedScreenPos(screenPoint), 0.0f)) };
        constraintMouse->SetTarget(pos);
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody);  // Use dummy body instead of rigidBody. It's better to create a dummy body automatically in ConstraintMouse2D
        constraintMouse->SetDampingRatio(0);
    }
    SubscribeToEvent(E_TOUCHMOVE, DRY_HANDLER(Constraints2D, HandleTouchMove3));
    SubscribeToEvent(E_TOUCHEND, DRY_HANDLER(Constraints2D, HandleTouchEnd3));
}

void Constraints2D::HandleTouchMove3(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode)
    {
        using namespace TouchMove;

        Graphics* graphics{ GetSubsystem<Graphics>() };
        ConstraintMouse2D* constraintMouse{ pickedNode->GetComponent<ConstraintMouse2D>() };
        IntVector2 screenPoint{ eventData[P_X].GetInt(), eventData[P_Y].GetInt() };
        Vector2 pos{ VectorTo2D(camera_->ScreenToWorldPos(graphics->NormalizedScreenPos(screenPoint), 0.0f)) };
        constraintMouse->SetTarget(pos);
    }
}

void Constraints2D::HandleTouchEnd3(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (pickedNode)
    {
        StaticSprite2D* staticSprite{ pickedNode->GetComponent<StaticSprite2D>() };
        staticSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // Restore picked sprite color

        pickedNode->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode = nullptr;
    }

    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
}
