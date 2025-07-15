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

/// Variable timestep scene update.
DRY_EVENT(E_SCENEUPDATE, SceneUpdate)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Scene subsystem update.
DRY_EVENT(E_SCENESUBSYSTEMUPDATE, SceneSubsystemUpdate)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Scene transform smoothing update.
DRY_EVENT(E_UPDATESMOOTHING, UpdateSmoothing)
{
    DRY_PARAM(P_CONSTANT, Constant);            // float
    DRY_PARAM(P_SQUAREDSNAPTHRESHOLD, SquaredSnapThreshold);  // float
}

/// Scene drawable update finished. Custom animation (eg. IK) can be done at this point.
DRY_EVENT(E_SCENEDRAWABLEUPDATEFINISHED, SceneDrawableUpdateFinished)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// SmoothedTransform target position changed.
DRY_EVENT(E_TARGETPOSITION, TargetPositionChanged)
{
}

/// SmoothedTransform target position changed.
DRY_EVENT(E_TARGETROTATION, TargetRotationChanged)
{
}

/// Scene attribute animation update.
DRY_EVENT(E_ATTRIBUTEANIMATIONUPDATE, AttributeAnimationUpdate)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Attribute animation added to object animation.
DRY_EVENT(E_ATTRIBUTEANIMATIONADDED, AttributeAnimationAdded)
{
    DRY_PARAM(P_OBJECTANIMATION, ObjectAnimation);               // Object animation pointer
    DRY_PARAM(P_ATTRIBUTEANIMATIONNAME, AttributeAnimationName); // String
}

/// Attribute animation removed from object animation.
DRY_EVENT(E_ATTRIBUTEANIMATIONREMOVED, AttributeAnimationRemoved)
{
    DRY_PARAM(P_OBJECTANIMATION, ObjectAnimation);               // Object animation pointer
    DRY_PARAM(P_ATTRIBUTEANIMATIONNAME, AttributeAnimationName); // String
}

/// Variable timestep scene post-update.
DRY_EVENT(E_SCENEPOSTUPDATE, ScenePostUpdate)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Asynchronous scene loading progress.
DRY_EVENT(E_ASYNCLOADPROGRESS, AsyncLoadProgress)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_PROGRESS, Progress);            // float
    DRY_PARAM(P_LOADEDNODES, LoadedNodes);      // int
    DRY_PARAM(P_TOTALNODES, TotalNodes);        // int
    DRY_PARAM(P_LOADEDRESOURCES, LoadedResources); // int
    DRY_PARAM(P_TOTALRESOURCES, TotalResources);   // int
}

/// Asynchronous scene loading finished.
DRY_EVENT(E_ASYNCLOADFINISHED, AsyncLoadFinished)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
}

/// A child node has been added to a parent node.
DRY_EVENT(E_NODEADDED, NodeAdded)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_PARENT, Parent);                // Node pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
}

/// A child node is about to be removed from a parent node. Note that individual component removal events will not be sent.
DRY_EVENT(E_NODEREMOVED, NodeRemoved)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_PARENT, Parent);                // Node pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
}

/// A component has been created to a node.
DRY_EVENT(E_COMPONENTADDED, ComponentAdded)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A component is about to be removed from a node.
DRY_EVENT(E_COMPONENTREMOVED, ComponentRemoved)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A node's name has changed.
DRY_EVENT(E_NODENAMECHANGED, NodeNameChanged)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
}

/// A node's enabled state has changed.
DRY_EVENT(E_NODEENABLEDCHANGED, NodeEnabledChanged)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
}

/// A node's tag has been added.
DRY_EVENT(E_NODETAGADDED, NodeTagAdded)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_TAG, Tag);                      // String tag
}

/// A node's tag has been removed.
DRY_EVENT(E_NODETAGREMOVED, NodeTagRemoved)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_TAG, Tag);                      // String tag
}

/// A component's enabled state has changed.
DRY_EVENT(E_COMPONENTENABLEDCHANGED, ComponentEnabledChanged)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A serializable's temporary state has changed.
DRY_EVENT(E_TEMPORARYCHANGED, TemporaryChanged)
{
    DRY_PARAM(P_SERIALIZABLE, Serializable);    // Serializable pointer
}

/// A node (and its children and components) has been cloned.
DRY_EVENT(E_NODECLONED, NodeCloned)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_NODE, Node);                    // Node pointer
    DRY_PARAM(P_CLONENODE, CloneNode);          // Node pointer
}

/// A component has been cloned.
DRY_EVENT(E_COMPONENTCLONED, ComponentCloned)
{
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_COMPONENT, Component);          // Component pointer
    DRY_PARAM(P_CLONECOMPONENT, CloneComponent); // Component pointer
}

/// A network attribute update from the server has been intercepted.
DRY_EVENT(E_INTERCEPTNETWORKUPDATE, InterceptNetworkUpdate)
{
    DRY_PARAM(P_SERIALIZABLE, Serializable);    // Serializable pointer
    DRY_PARAM(P_TIMESTAMP, TimeStamp);          // unsigned (0-255)
    DRY_PARAM(P_INDEX, Index);                  // unsigned
    DRY_PARAM(P_NAME, Name);                    // String
    DRY_PARAM(P_VALUE, Value);                  // Variant
}

}
