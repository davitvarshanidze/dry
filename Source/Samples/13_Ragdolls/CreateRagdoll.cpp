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

#include <Dry/Graphics/AnimatedModel.h>
#include <Dry/IO/Log.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/RigidBody.h>

#include "CreateRagdoll.h"

#include <Dry/DebugNew.h>

CreateRagdoll::CreateRagdoll(Context* context) :
    Component(context)
{
}

void CreateRagdoll::OnNodeSet(Node* node)
{
    // If the node pointer is non-null, this component has been created into a scene node. Subscribe to physics collisions that
    // concern this scene node
    if (node)
        SubscribeToEvent(node, E_NODECOLLISION, DRY_HANDLER(CreateRagdoll, HandleNodeCollision));
}

void CreateRagdoll::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;

    // Get the other colliding body, make sure it is moving (has nonzero mass)
    RigidBody* otherBody{ static_cast<RigidBody*>(eventData[P_OTHERBODY].GetPtr()) };

    if (otherBody->GetMass() > 0.0f)
    {
        // We do not need the physics components in the AnimatedModel's root scene node anymore
        node_->RemoveComponent<RigidBody>();
        node_->RemoveComponent<CollisionShape>();

        ResourceCache* cache{ GetSubsystem<ResourceCache>() };
        CopyPhysicsComponents(cache->GetResource<XMLFile>("Objects/Robo.xml"), node_);

        // Disable keyframe animation from all bones so that they will not interfere with the ragdoll
        AnimatedModel* model = GetComponent<AnimatedModel>();
        Skeleton& skeleton = model->GetSkeleton();
        for (unsigned i{ 0 }; i < skeleton.GetNumBones(); ++i)
            skeleton.GetBone(i)->animated_ = false;

        // Finally remove self from the scene node. Note that this must be the last operation performed in the function
        Remove();
    }
}

void CreateRagdoll::CopyPhysicsComponents(XMLFile* prefab, Node* rootNode)
{
    const XMLElement rootElem{ prefab->GetRoot() };

    if (!rootElem)
        return;

    assert(rootElem.GetName() == "node");

    CopyPhysicsComponents(rootElem, node_);
}

void CreateRagdoll::CopyPhysicsComponents(const XMLElement& from, Node* to, bool recursive)
{
    XMLElement componentElem{ from.GetChild("component") };
    const Matrix3x4 oldTransform{ to->GetTransform() };

    while (componentElem)
    {
        const String typeString{ componentElem.GetAttribute("type") };

        if (typeString == "RigidBody")
        {
            XMLElement attrElem{ componentElem.GetChild("attribute") };
            float mass{ 0.0f };
            while (attrElem)
            {
                if (attrElem.GetAttribute("name") == "Mass")
                {
                    mass = attrElem.GetFloat("value");
                    break;
                }

                attrElem = attrElem.GetNext("attribute");
            }

            RigidBody* body{ to->CreateComponent<RigidBody>() };
            body->SetMass(mass);
            body->ApplyAttributes();
        }
        else if (typeString == "CollisionShape")
        {
            CollisionShape* shape{ to->CreateComponent<CollisionShape>() };
            shape->LoadXML(componentElem);
            shape->ApplyAttributes();
        }
        else if (typeString == "Constraint")
        {
            Constraint* constraint{ to->CreateComponent<Constraint>() };
            constraint->LoadXML(componentElem);
            constraint->ApplyAttributes();
        }

        componentElem = componentElem.GetNext("component");
    }


    // Recurse nodes by name
    if (recursive)
    {
        XMLElement nodeElem{ from.GetChild("node") };

        while (nodeElem)
        {
            String nodeName{};
            XMLElement nodeAttrElem{ nodeElem.GetChild("attribute") };

            while (nodeAttrElem)
            {
                if (nodeAttrElem.GetAttribute("name") == "Name")
                {
                    nodeName = nodeAttrElem.GetAttribute("value");
                    break;
                }

                nodeAttrElem = nodeAttrElem.GetNext("attribute");
            }

            if (!nodeName.IsEmpty())
            {
                Node* bone{ to->GetChild(nodeName) };

                if (bone)
                    CopyPhysicsComponents(nodeElem, bone, true);
            }

            nodeElem = nodeElem.GetNext("node");
        }
    }

    to->MarkDirty();
}
