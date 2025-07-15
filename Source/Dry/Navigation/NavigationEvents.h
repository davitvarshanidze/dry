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

/// Complete rebuild of navigation mesh.
DRY_EVENT(E_NAVIGATION_MESH_REBUILT, NavigationMeshRebuilt)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Partial bounding box rebuild of navigation mesh.
DRY_EVENT(E_NAVIGATION_AREA_REBUILT, NavigationAreaRebuilt)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DRY_PARAM(P_BOUNDSMIN, BoundsMin); // Vector3
    DRY_PARAM(P_BOUNDSMAX, BoundsMax); // Vector3
}

/// Mesh tile is added to navigation mesh.
DRY_EVENT(E_NAVIGATION_TILE_ADDED, NavigationTileAdded)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DRY_PARAM(P_TILE, Tile); // IntVector2
}

/// Mesh tile is removed from navigation mesh.
DRY_EVENT(E_NAVIGATION_TILE_REMOVED, NavigationTileRemoved)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DRY_PARAM(P_TILE, Tile); // IntVector2
}

/// All mesh tiles are removed from navigation mesh.
DRY_EVENT(E_NAVIGATION_ALL_TILES_REMOVED, NavigationAllTilesRemoved)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Crowd agent formation.
DRY_EVENT(E_CROWD_AGENT_FORMATION, CrowdAgentFormation)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_INDEX, Index); // unsigned
    DRY_PARAM(P_SIZE, Size); // unsigned
    DRY_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent formation specific to a node.
DRY_EVENT(E_CROWD_AGENT_NODE_FORMATION, CrowdAgentNodeFormation)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_INDEX, Index); // unsigned
    DRY_PARAM(P_SIZE, Size); // unsigned
    DRY_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent has been repositioned.
DRY_EVENT(E_CROWD_AGENT_REPOSITION, CrowdAgentReposition)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_ARRIVED, Arrived); // bool
    DRY_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent has been repositioned, specific to a node
DRY_EVENT(E_CROWD_AGENT_NODE_REPOSITION, CrowdAgentNodeReposition)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_ARRIVED, Arrived); // bool
    DRY_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
DRY_EVENT(E_CROWD_AGENT_FAILURE, CrowdAgentFailure)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DRY_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
DRY_EVENT(E_CROWD_AGENT_NODE_FAILURE, CrowdAgentNodeFailure)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DRY_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
DRY_EVENT(E_CROWD_AGENT_STATE_CHANGED, CrowdAgentStateChanged)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DRY_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
DRY_EVENT(E_CROWD_AGENT_NODE_STATE_CHANGED, CrowdAgentNodeStateChanged)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_VELOCITY, Velocity); // Vector3
    DRY_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DRY_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Addition of obstacle to dynamic navigation mesh.
DRY_EVENT(E_NAVIGATION_OBSTACLE_ADDED, NavigationObstacleAdded)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_RADIUS, Radius); // float
    DRY_PARAM(P_HEIGHT, Height); // float
}

/// Removal of obstacle from dynamic navigation mesh.
DRY_EVENT(E_NAVIGATION_OBSTACLE_REMOVED, NavigationObstacleRemoved)
{
    DRY_PARAM(P_NODE, Node); // Node pointer
    DRY_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    DRY_PARAM(P_POSITION, Position); // Vector3
    DRY_PARAM(P_RADIUS, Radius); // float
    DRY_PARAM(P_HEIGHT, Height); // float
}

}
