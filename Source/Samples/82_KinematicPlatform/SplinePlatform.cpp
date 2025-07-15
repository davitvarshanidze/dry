//
// Copyright (c) 2008-2019 the Urho3D project.
// Copyright (c) 2022-2022 LucKey Productions.
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

#include <Dry/Core/Context.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RigidBody.h>
#include <Dry/Scene/SplinePath.h>
#include <Dry/IO/Log.h>

#include "SplinePlatform.h"

#include <Dry/DebugNew.h>


SplinePlatform::SplinePlatform(Context* context): LogicComponent(context),
    rotation_{ 0.023f }
{
    SetUpdateEventMask(USE_NO_EVENT);
}

SplinePlatform::~SplinePlatform()
{
}

void SplinePlatform::RegisterObject(Context* context)
{
    context->RegisterFactory<SplinePlatform>();
}

void SplinePlatform::Initialize(Node* node)
{
    splinePathNode_ = node;
    splinePath_ = splinePathNode_->GetComponent<SplinePath>();
    controlNode_ = splinePath_->GetControlledNode();
    splinePath_->SetSpeed(2.3f);

    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void SplinePlatform::FixedUpdate(float timeStep)
{
    if (splinePath_)
    {
        splinePath_->Move(timeStep);

        // Looped path, reset to continue
        if (splinePath_->IsFinished())
            splinePath_->Reset();

        // Rotate
        if (controlNode_)
        {
            const Quaternion drot{ rotation_, Vector3::UP };
            const Quaternion nrot{ controlNode_->GetWorldRotation() };
            controlNode_->SetWorldRotation(nrot * drot);
        }
    }
}




