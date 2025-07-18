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

#pragma once

#include "../Core/Object.h"

namespace Dry
{

/// Physics world is about to be stepped.
DRY_EVENT(E_PHYSICSPRESTEP, PhysicsPreStep)
{
    DRY_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics world has been stepped.
DRY_EVENT(E_PHYSICSPOSTSTEP, PhysicsPostStep)
{
    DRY_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics collision started. Global event sent by the PhysicsWorld.
DRY_EVENT(E_PHYSICSCOLLISIONSTART, PhysicsCollisionStart)
{
    DRY_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DRY_PARAM(P_NODEA, NodeA);                  // Node pointer
    DRY_PARAM(P_NODEB, NodeB);                  // Node pointer
    DRY_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DRY_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
    DRY_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ongoing. Global event sent by the PhysicsWorld.
DRY_EVENT(E_PHYSICSCOLLISION, PhysicsCollision)
{
    DRY_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DRY_PARAM(P_NODEA, NodeA);                  // Node pointer
    DRY_PARAM(P_NODEB, NodeB);                  // Node pointer
    DRY_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DRY_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
    DRY_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ended. Global event sent by the PhysicsWorld.
DRY_EVENT(E_PHYSICSCOLLISIONEND, PhysicsCollisionEnd)
{
    DRY_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DRY_PARAM(P_NODEA, NodeA);                  // Node pointer
    DRY_PARAM(P_NODEB, NodeB);                  // Node pointer
    DRY_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DRY_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
}

/// Node's physics collision started. Sent by scene nodes participating in a collision.
DRY_EVENT(E_NODECOLLISIONSTART, NodeCollisionStart)
{
    DRY_PARAM(P_BODY, Body);                    // RigidBody pointer
    DRY_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DRY_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
    DRY_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ongoing. Sent by scene nodes participating in a collision.
DRY_EVENT(E_NODECOLLISION, NodeCollision)
{
    DRY_PARAM(P_BODY, Body);                    // RigidBody pointer
    DRY_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DRY_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
    DRY_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ended. Sent by scene nodes participating in a collision.
DRY_EVENT(E_NODECOLLISIONEND, NodeCollisionEnd)
{
    DRY_PARAM(P_BODY, Body);                    // RigidBody pointer
    DRY_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DRY_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DRY_PARAM(P_TRIGGER, Trigger);              // bool
}

}
