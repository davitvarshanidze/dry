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

#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/Constraint.h>
#include <Dry/Resource/XMLFile.h>

using namespace Dry;


/// Custom component that creates a ragdoll upon collision.
class CreateRagdoll : public Component
{
    DRY_OBJECT(CreateRagdoll, Component);

public:
    /// Construct.
    explicit CreateRagdoll(Context* context);

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* node) override;

private:
    /// Handle scene node's physics collision.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);

    void CopyPhysicsComponents(XMLFile* prefab, Node* rootNode);
    void CopyPhysicsComponents(const XMLElement& from, Node* to, bool recursive = true);

};
