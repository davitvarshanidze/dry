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

/// \file

#pragma once

#include "../Scene/Animatable.h"

namespace Dry
{

class DebugRenderer;
class Node;
class Scene;

struct ComponentReplicationState;

/// Autoremove is used by some components for automatic removal from the scene hierarchy upon completion of an action, for example sound or particle effect.
enum AutoRemoveMode
{
    REMOVE_DISABLED = 0,
    REMOVE_COMPONENT,
    REMOVE_NODE
};

/// Base class for components. Components can be created to scene nodes.
class DRY_API Component : public Animatable
{
    DRY_OBJECT(Component, Animatable);

    friend class Node;
    friend class Scene;

public:
    /// Construct.
    explicit Component(Context* context);
    ~Component() override = default;

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled() { }

    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;
    /// Mark for attribute check on the next network update.
    void MarkNetworkUpdate() override;
    /// Return the depended on nodes to order network updates.
    virtual void GetDependencyNodes(PODVector<Node*>& dest);
    /// Visualize the component as debug geometry.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    /// Set enabled/disabled state.
    void SetEnabled(bool enable);
    /// Remove from the scene node. If no other shared pointer references exist, causes immediate deletion.
    void Remove();

    /// Return ID.
    unsigned GetID() const { return id_; }
    /// Return whether the component is replicated or local to a scene.
    bool IsReplicated() const;
    /// Return whether is enabled.
    bool IsEnabled() const { return enabled_; }
    /// Return whether is effectively enabled (node is also enabled).
    bool IsEnabledEffective() const;

    /// Return scene node.
    Node* GetNode() const { return node_; }
    /// Return the scene the node belongs to.
    Scene* GetScene() const;
    /// Return component in the same scene node by first of type.
    Component* GetComponent(StringHash type) const;
    /// Return components in the same scene node by type.
    void GetComponents(PODVector<Component*>& dest, StringHash type) const;
    /// Template version of returning component in the same scene node by first of type.
    template <class T> T* GetComponent() const;
    /// Template version of returning components in the same scene node by type.
    template <class T> void GetComponents(PODVector<T*>& dest) const;

    /// Add a replication state that is tracking this component.
    void AddReplicationState(ComponentReplicationState* state);
    /// Prepare network update by comparing attributes and marking replication states dirty as necessary.
    void PrepareNetworkUpdate();
    /// Clean up all references to a network connection that is about to be removed.
    void CleanupConnection(Connection* connection);

protected:
    /// Handle attribute animation added.
    void OnAttributeAnimationAdded() override;
    /// Handle attribute animation removed.
    void OnAttributeAnimationRemoved() override;
    /// Handle scene node being assigned at creation.
    virtual void OnNodeSet(Node* node);
    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene);
    /// Handle scene node transform dirtied.
    virtual void OnMarkedDirty(Node* node);
    /// Handle scene node enabled status changing.
    virtual void OnNodeSetEnabled(Node* node);
    /// Set ID. Called by Scene.
    void SetID(unsigned id);
    /// Set scene node. Called by Node when creating the component.
    void SetNode(Node* node);
    /// Handle scene attribute animation update event.
    void HandleAttributeAnimationUpdate(StringHash eventType, VariantMap& eventData);
    /// Return a component from the scene root that sends out fixed update events (either PhysicsWorld or PhysicsWorld2D). Return null if neither exists.
    Component* GetFixedUpdateSource();
    /// Perform autoremove. Called by subclasses. Caller should keep a weak pointer to itself to check whether was actually removed, and return immediately without further member operations in that case.
    void DoAutoRemove(AutoRemoveMode mode);

    /// Scene node.
    Node* node_;
    /// Unique ID within the scene.
    unsigned id_;
    /// Network update queued flag.
    bool networkUpdate_;
    /// Enabled flag.
    bool enabled_;
};

template <class T> T* Component::GetComponent() const { return static_cast<T*>(GetComponent(T::GetTypeStatic())); }

template <class T> void Component::GetComponents(PODVector<T*>& dest) const
{
    GetComponents(reinterpret_cast<PODVector<Component*>&>(dest), T::GetTypeStatic());
}

}
