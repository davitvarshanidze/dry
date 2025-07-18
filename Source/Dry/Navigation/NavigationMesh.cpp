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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Model.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/TerrainPatch.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Navigation/CrowdAgent.h"
#include "../Navigation/DynamicNavigationMesh.h"
#include "../Navigation/NavArea.h"
#include "../Navigation/NavBuildData.h"
#include "../Navigation/Navigable.h"
#include "../Navigation/NavigationEvents.h"
#include "../Navigation/NavigationMesh.h"
#include "../Navigation/Obstacle.h"
#include "../Navigation/OffMeshConnection.h"
#ifdef DRY_PHYSICS
#include "../Physics/CollisionShape.h"
#endif
#include "../Scene/Scene.h"

#include <cfloat>
#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourNavMeshQuery.h>
#include <Recast/Recast.h>

#include "../DebugNew.h"

namespace Dry
{

const char* navmeshPartitionTypeNames[] =
{
    "watershed",
    "monotone",
    nullptr
};

const char* DRY_NAVIGATION_CATEGORY = "Navigation";

static const int DEFAULT_TILE_SIZE = 128;
static const float DEFAULT_CELL_SIZE = 0.3f;
static const float DEFAULT_CELL_HEIGHT = 0.2f;
static const float DEFAULT_AGENT_HEIGHT = 2.0f;
static const float DEFAULT_AGENT_RADIUS = 0.6f;
static const float DEFAULT_AGENT_MAX_CLIMB = 0.9f;
static const float DEFAULT_AGENT_MAX_SLOPE = 45.0f;
static const float DEFAULT_REGION_MIN_SIZE = 8.0f;
static const float DEFAULT_REGION_MERGE_SIZE = 20.0f;
static const float DEFAULT_EDGE_MAX_LENGTH = 12.0f;
static const float DEFAULT_EDGE_MAX_ERROR = 1.3f;
static const float DEFAULT_DETAIL_SAMPLE_DISTANCE = 6.0f;
static const float DEFAULT_DETAIL_SAMPLE_MAX_ERROR = 1.0f;

static const int MAX_POLYS = 2048;


/// Temporary data for finding a path.
struct FindPathData
{
    // Polygons.
    dtPolyRef polys_[MAX_POLYS]{};
    // Polygons on the path.
    dtPolyRef pathPolys_[MAX_POLYS]{};
    // Points on the path.
    Vector3 pathPoints_[MAX_POLYS];
    // Flags on the path.
    unsigned char pathFlags_[MAX_POLYS]{};
};

NavigationMesh::NavigationMesh(Context* context) :
    Component(context),
    navMesh_(nullptr),
    navMeshQuery_(nullptr),
    queryFilter_(new dtQueryFilter()),
    pathData_(new FindPathData()),
    tileSize_(DEFAULT_TILE_SIZE),
    cellSize_(DEFAULT_CELL_SIZE),
    cellHeight_(DEFAULT_CELL_HEIGHT),
    agentHeight_(DEFAULT_AGENT_HEIGHT),
    agentRadius_(DEFAULT_AGENT_RADIUS),
    agentMaxClimb_(DEFAULT_AGENT_MAX_CLIMB),
    agentMaxSlope_(DEFAULT_AGENT_MAX_SLOPE),
    regionMinSize_(DEFAULT_REGION_MIN_SIZE),
    regionMergeSize_(DEFAULT_REGION_MERGE_SIZE),
    edgeMaxLength_(DEFAULT_EDGE_MAX_LENGTH),
    edgeMaxError_(DEFAULT_EDGE_MAX_ERROR),
    detailSampleDistance_(DEFAULT_DETAIL_SAMPLE_DISTANCE),
    detailSampleMaxError_(DEFAULT_DETAIL_SAMPLE_MAX_ERROR),
    padding_(Vector3::ONE),
    numTilesX_(0),
    numTilesZ_(0),
    partitionType_(NAVMESH_PARTITION_WATERSHED),
    keepInterResults_(false),
    drawOffMeshConnections_(false),
    drawNavAreas_(false)
{
}

NavigationMesh::~NavigationMesh()
{
    ReleaseNavigationMesh();
}

void NavigationMesh::RegisterObject(Context* context)
{
    context->RegisterFactory<NavigationMesh>(DRY_NAVIGATION_CATEGORY);

    DRY_ACCESSOR_ATTRIBUTE("Tile Size", GetTileSize, SetTileSize, int, DEFAULT_TILE_SIZE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Cell Size", GetCellSize, SetCellSize, float, DEFAULT_CELL_SIZE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Cell Height", GetCellHeight, SetCellHeight, float, DEFAULT_CELL_HEIGHT, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Agent Height", GetAgentHeight, SetAgentHeight, float, DEFAULT_AGENT_HEIGHT, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Agent Radius", GetAgentRadius, SetAgentRadius, float, DEFAULT_AGENT_RADIUS, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Agent Max Climb", GetAgentMaxClimb, SetAgentMaxClimb, float, DEFAULT_AGENT_MAX_CLIMB, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Agent Max Slope", GetAgentMaxSlope, SetAgentMaxSlope, float, DEFAULT_AGENT_MAX_SLOPE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Region Min Size", GetRegionMinSize, SetRegionMinSize, float, DEFAULT_REGION_MIN_SIZE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Region Merge Size", GetRegionMergeSize, SetRegionMergeSize, float, DEFAULT_REGION_MERGE_SIZE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Edge Max Length", GetEdgeMaxLength, SetEdgeMaxLength, float, DEFAULT_EDGE_MAX_LENGTH, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Edge Max Error", GetEdgeMaxError, SetEdgeMaxError, float, DEFAULT_EDGE_MAX_ERROR, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Detail Sample Distance", GetDetailSampleDistance, SetDetailSampleDistance, float,
        DEFAULT_DETAIL_SAMPLE_DISTANCE, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Detail Sample Max Error", GetDetailSampleMaxError, SetDetailSampleMaxError, float,
        DEFAULT_DETAIL_SAMPLE_MAX_ERROR, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Bounding Box Padding", GetPadding, SetPadding, Vector3, Vector3::ONE, AM_DEFAULT);
    DRY_MIXED_ACCESSOR_ATTRIBUTE("Navigation Data", GetNavigationDataAttr, SetNavigationDataAttr, PODVector<unsigned char>,
        Variant::emptyBuffer, AM_FILE | AM_NOEDIT);
    DRY_ENUM_ACCESSOR_ATTRIBUTE("Partition Type", GetPartitionType, SetPartitionType, NavmeshPartitionType, navmeshPartitionTypeNames,
        NAVMESH_PARTITION_WATERSHED, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Draw OffMeshConnections", GetDrawOffMeshConnections, SetDrawOffMeshConnections, bool, false, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Draw NavAreas", GetDrawNavAreas, SetDrawNavAreas, bool, false, AM_DEFAULT);
}

void NavigationMesh::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!debug || !navMesh_ || !node_)
        return;

    const Matrix3x4& worldTransform = node_->GetWorldTransform();

    const dtNavMesh* navMesh = navMesh_;

    for (int j{ 0 }; j < navMesh->getMaxTiles(); ++j)
    {
        const dtMeshTile* tile = navMesh->getTile(j);
        assert(tile);
        if (!tile->header)
            continue;

        for (int i{ 0 }; i < tile->header->polyCount; ++i)
        {
            dtPoly* poly = tile->polys + i;
            for (unsigned j{ 0 }; j < poly->vertCount; ++j)
            {
                debug->AddLine(
                    worldTransform * *reinterpret_cast<const Vector3*>(&tile->verts[poly->verts[j] * 3]),
                    worldTransform * *reinterpret_cast<const Vector3*>(&tile->verts[poly->verts[(j + 1) % poly->vertCount] * 3]),
                    Color::YELLOW,
                    depthTest
                );
            }
        }
    }

    Scene* scene = GetScene();
    if (scene)
    {
        // Draw OffMeshConnection components
        if (drawOffMeshConnections_)
        {
            PODVector<Node*> connections;
            scene->GetChildrenWithComponent<OffMeshConnection>(connections, true);
            for (unsigned i{ 0 }; i < connections.Size(); ++i)
            {
                auto* connection = connections[i]->GetComponent<OffMeshConnection>();
                if (connection && connection->IsEnabledEffective())
                    connection->DrawDebugGeometry(debug, depthTest);
            }
        }

        // Draw NavArea components
        if (drawNavAreas_)
        {
            for (unsigned i{ 0 }; i < areas_.Size(); ++i)
            {
                NavArea* area = areas_[i];
                if (area && area->IsEnabledEffective())
                    area->DrawDebugGeometry(debug, depthTest);
            }
        }
    }
}

void NavigationMesh::SetMeshName(const String& newName)
{
    meshName_ = newName;
}

void NavigationMesh::SetTileSize(int size)
{
    tileSize_ = Max(size, 16);

    MarkNetworkUpdate();
}

void NavigationMesh::SetCellSize(float size)
{
    cellSize_ = Max(size, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetCellHeight(float height)
{
    cellHeight_ = Max(height, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetAgentHeight(float height)
{
    agentHeight_ = Max(height, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetAgentRadius(float radius)
{
    agentRadius_ = Max(radius, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetAgentMaxClimb(float maxClimb)
{
    agentMaxClimb_ = Max(maxClimb, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetAgentMaxSlope(float maxSlope)
{
    agentMaxSlope_ = Max(maxSlope, 0.0f);

    MarkNetworkUpdate();
}

void NavigationMesh::SetRegionMinSize(float size)
{
    regionMinSize_ = Max(size, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetRegionMergeSize(float size)
{
    regionMergeSize_ = Max(size, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetEdgeMaxLength(float length)
{
    edgeMaxLength_ = Max(length, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetEdgeMaxError(float error)
{
    edgeMaxError_ = Max(error, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetDetailSampleDistance(float distance)
{
    detailSampleDistance_ = Max(distance, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetDetailSampleMaxError(float error)
{
    detailSampleMaxError_ = Max(error, M_EPSILON);

    MarkNetworkUpdate();
}

void NavigationMesh::SetPadding(const Vector3& padding)
{
    padding_ = padding;

    MarkNetworkUpdate();
}

bool NavigationMesh::Allocate(const BoundingBox& boundingBox, unsigned maxTiles)
{
    // Release existing navigation data and zero the bounding box
    ReleaseNavigationMesh();

    if (!node_)
        return false;

    if (!node_->GetWorldScale().Equals(Vector3::ONE))
        DRY_LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

    boundingBox_ = boundingBox.Transformed(node_->GetWorldTransform().Inverse());
    maxTiles = NextPowerOfTwo(maxTiles);

    // Calculate number of tiles
    int gridW = 0, gridH = 0;
    float tileEdgeLength = (float)tileSize_ * cellSize_;
    rcCalcGridSize(&boundingBox_.min_.x_, &boundingBox_.max_.x_, cellSize_, &gridW, &gridH);
    numTilesX_ = (gridW + tileSize_ - 1) / tileSize_;
    numTilesZ_ = (gridH + tileSize_ - 1) / tileSize_;

    // Calculate max number of polygons, 22 bits available to identify both tile & polygon within tile
    unsigned tileBits = LogBaseTwo(maxTiles);
    unsigned maxPolys = 1u << (22 - tileBits);

    dtNavMeshParams params;     // NOLINT(hicpp-member-init)
    rcVcopy(params.orig, &boundingBox_.min_.x_);
    params.tileWidth = tileEdgeLength;
    params.tileHeight = tileEdgeLength;
    params.maxTiles = maxTiles;
    params.maxPolys = maxPolys;

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        DRY_LOGERROR("Could not allocate navigation mesh");
        return false;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        DRY_LOGERROR("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return false;
    }

    DRY_LOGDEBUG("Allocated empty navigation mesh with max " + String(maxTiles) + " tiles");

    // Send a notification event to concerned parties that we've been fully rebuilt
    {
        using namespace NavigationMeshRebuilt;
        VariantMap& buildEventParams = GetContext()->GetEventDataMap();
        buildEventParams[P_NODE] = node_;
        buildEventParams[P_MESH] = this;
        SendEvent(E_NAVIGATION_MESH_REBUILT, buildEventParams);
    }
    return true;
}

bool NavigationMesh::Build()
{
    DRY_PROFILE(BuildNavigationMesh);

    // Release existing navigation data and zero the bounding box
    ReleaseNavigationMesh();

    if (!node_)
        return false;

    if (!node_->GetWorldScale().Equals(Vector3::ONE))
        DRY_LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

    Vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    if (geometryList.IsEmpty())
        return true; // Nothing to do

    // Build the combined bounding box
    for (unsigned i{ 0 }; i < geometryList.Size(); ++i)
        boundingBox_.Merge(geometryList[i].boundingBox_);

    // Expand bounding box by padding
    boundingBox_.min_ -= padding_;
    boundingBox_.max_ += padding_;

    {
        DRY_PROFILE(BuildNavigationMesh);

        // Calculate number of tiles
        int gridW = 0, gridH = 0;
        float tileEdgeLength = (float)tileSize_ * cellSize_;
        rcCalcGridSize(&boundingBox_.min_.x_, &boundingBox_.max_.x_, cellSize_, &gridW, &gridH);
        numTilesX_ = (gridW + tileSize_ - 1) / tileSize_;
        numTilesZ_ = (gridH + tileSize_ - 1) / tileSize_;

        // Calculate max. number of tiles and polygons, 22 bits available to identify both tile & polygon within tile
        unsigned maxTiles = NextPowerOfTwo((unsigned)(numTilesX_ * numTilesZ_));
        unsigned tileBits = LogBaseTwo(maxTiles);
        unsigned maxPolys = 1u << (22 - tileBits);

        dtNavMeshParams params;     // NOLINT(hicpp-member-init)
        rcVcopy(params.orig, &boundingBox_.min_.x_);
        params.tileWidth = tileEdgeLength;
        params.tileHeight = tileEdgeLength;
        params.maxTiles = maxTiles;
        params.maxPolys = maxPolys;

        navMesh_ = dtAllocNavMesh();
        if (!navMesh_)
        {
            DRY_LOGERROR("Could not allocate navigation mesh");
            return false;
        }

        if (dtStatusFailed(navMesh_->init(&params)))
        {
            DRY_LOGERROR("Could not initialize navigation mesh");
            ReleaseNavigationMesh();
            return false;
        }

        // Build each tile
        unsigned numTiles = BuildTiles(geometryList, IntVector2::ZERO, GetNumTiles() - IntVector2::ONE);

        DRY_LOGDEBUG("Built navigation mesh with " + String(numTiles) + " tiles");

        // Send a notification event to concerned parties that we've been fully rebuilt
        {
            using namespace NavigationMeshRebuilt;
            VariantMap& buildEventParams = GetContext()->GetEventDataMap();
            buildEventParams[P_NODE] = node_;
            buildEventParams[P_MESH] = this;
            SendEvent(E_NAVIGATION_MESH_REBUILT, buildEventParams);
        }

        return true;
    }
}

bool NavigationMesh::Build(const BoundingBox& boundingBox)
{
    DRY_PROFILE(BuildPartialNavigationMesh);

    if (!node_)
        return false;

    if (!navMesh_)
    {
        DRY_LOGERROR("Navigation mesh must first be built fully before it can be partially rebuilt");
        return false;
    }

    if (!node_->GetWorldScale().Equals(Vector3::ONE))
        DRY_LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

    BoundingBox localSpaceBox = boundingBox.Transformed(node_->GetWorldTransform().Inverse());

    float tileEdgeLength = (float)tileSize_ * cellSize_;

    Vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    int sx = Clamp((int)((localSpaceBox.min_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
    int sz = Clamp((int)((localSpaceBox.min_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);
    int ex = Clamp((int)((localSpaceBox.max_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
    int ez = Clamp((int)((localSpaceBox.max_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);

    unsigned numTiles = BuildTiles(geometryList, IntVector2(sx, sz), IntVector2(ex, ez));

    DRY_LOGDEBUG("Rebuilt " + String(numTiles) + " tiles of the navigation mesh");
    return true;
}

bool NavigationMesh::Build(const IntVector2& from, const IntVector2& to)
{
    DRY_PROFILE(BuildPartialNavigationMesh);

    if (!node_)
        return false;

    if (!navMesh_)
    {
        DRY_LOGERROR("Navigation mesh must first be built fully before it can be partially rebuilt");
        return false;
    }

    if (!node_->GetWorldScale().Equals(Vector3::ONE))
        DRY_LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

    Vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    unsigned numTiles = BuildTiles(geometryList, from, to);

    DRY_LOGDEBUG("Rebuilt " + String(numTiles) + " tiles of the navigation mesh");
    return true;
}

PODVector<unsigned char> NavigationMesh::GetTileData(const IntVector2& tile) const
{
    VectorBuffer ret;
    WriteTile(ret, tile.x_, tile.y_);
    return ret.GetBuffer();
}

bool NavigationMesh::AddTile(const PODVector<unsigned char>& tileData)
{
    MemoryBuffer buffer(tileData);
    return ReadTile(buffer, false);
}

bool NavigationMesh::HasTile(const IntVector2& tile) const
{
    if (navMesh_)
        return !!navMesh_->getTileAt(tile.x_, tile.y_, 0);
    return false;
}

BoundingBox NavigationMesh::GetTileBoundingBox(const IntVector2& tile) const
{
    const float tileEdgeLength = (float)tileSize_ * cellSize_;
    return BoundingBox(
        Vector3(
            boundingBox_.min_.x_ + tileEdgeLength * (float)tile.x_,
            boundingBox_.min_.y_,
            boundingBox_.min_.z_ + tileEdgeLength * (float)tile.y_
        ),
        Vector3(
            boundingBox_.min_.x_ + tileEdgeLength * (float)(tile.x_ + 1),
            boundingBox_.max_.y_,
            boundingBox_.min_.z_ + tileEdgeLength * (float)(tile.y_ + 1)
        ));
}

IntVector2 NavigationMesh::GetTileIndex(const Vector3& position) const
{
    const float tileEdgeLength = (float)tileSize_ * cellSize_;
    const Vector3 localPosition = node_->GetWorldTransform().Inverse() * position - boundingBox_.min_;
    const Vector2 localPosition2D(localPosition.x_, localPosition.z_);
    return VectorMin(VectorMax(IntVector2::ZERO, VectorFloorToInt(localPosition2D / tileEdgeLength)), GetNumTiles() - IntVector2::ONE);
}

void NavigationMesh::RemoveTile(const IntVector2& tile)
{
    if (!navMesh_)
        return;

    const dtTileRef tileRef = navMesh_->getTileRefAt(tile.x_, tile.y_, 0);
    if (!tileRef)
        return;

    navMesh_->removeTile(tileRef, nullptr, nullptr);

    // Send event
    using namespace NavigationTileRemoved;
    VariantMap& eventData = GetContext()->GetEventDataMap();
    eventData[P_NODE] = GetNode();
    eventData[P_MESH] = this;
    eventData[P_TILE] = tile;
    SendEvent(E_NAVIGATION_TILE_REMOVED, eventData);
}

void NavigationMesh::RemoveAllTiles()
{
    const dtNavMesh* navMesh = navMesh_;
    for (int i{ 0 }; i < navMesh_->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        assert(tile);
        if (tile->header)
            navMesh_->removeTile(navMesh_->getTileRef(tile), nullptr, nullptr);
    }

    // Send event
    using namespace NavigationAllTilesRemoved;
    VariantMap& eventData = GetContext()->GetEventDataMap();
    eventData[P_NODE] = GetNode();
    eventData[P_MESH] = this;
    SendEvent(E_NAVIGATION_ALL_TILES_REMOVED, eventData);
}

Vector3 NavigationMesh::FindNearestPoint(const Vector3& point, const Vector3& extents, const dtQueryFilter* filter,
    dtPolyRef* nearestRef)
{
    if (!InitializeQuery())
        return point;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localPoint = inverse * point;
    Vector3 nearestPoint;

    dtPolyRef pointRef;
    if (!nearestRef)
        nearestRef = &pointRef;
    navMeshQuery_->findNearestPoly(&localPoint.x_, &extents.x_, filter ? filter : queryFilter_.Get(), nearestRef, &nearestPoint.x_);
    return *nearestRef ? transform * nearestPoint : point;
}

Vector3 NavigationMesh::MoveAlongSurface(const Vector3& start, const Vector3& end, const Vector3& extents, int maxVisited,
    const dtQueryFilter* filter)
{
    if (!InitializeQuery())
        return end;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.Get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return end;

    Vector3 resultPos;
    int visitedCount = 0;
    maxVisited = Max(maxVisited, 0);
    PODVector<dtPolyRef> visited((unsigned)maxVisited);
    navMeshQuery_->moveAlongSurface(startRef, &localStart.x_, &localEnd.x_, queryFilter, &resultPos.x_, maxVisited ?
        &visited[0] : nullptr, &visitedCount, maxVisited);
    return transform * resultPos;
}

void NavigationMesh::FindPath(PODVector<Vector3>& dest, const Vector3& start, const Vector3& end, const Vector3& extents,
    const dtQueryFilter* filter)
{
    PODVector<NavigationPathPoint> navPathPoints;
    FindPath(navPathPoints, start, end, extents, filter);

    dest.Clear();
    for (unsigned i{ 0 }; i < navPathPoints.Size(); ++i)
        dest.Push(navPathPoints[i].position_);
}

void NavigationMesh::FindPath(PODVector<NavigationPathPoint>& dest, const Vector3& start, const Vector3& end,
    const Vector3& extents, const dtQueryFilter* filter)
{
    DRY_PROFILE(FindPath);
    dest.Clear();

    if (!InitializeQuery())
        return;

    // Navigation data is in local space. Transform path points from world to local
    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.Get();
    dtPolyRef startRef;
    dtPolyRef endRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    navMeshQuery_->findNearestPoly(&localEnd.x_, &extents.x_, queryFilter, &endRef, nullptr);

    if (!startRef || !endRef)
        return;

    int numPolys = 0;
    int numPathPoints = 0;

    navMeshQuery_->findPath(startRef, endRef, &localStart.x_, &localEnd.x_, queryFilter, pathData_->polys_, &numPolys,
        MAX_POLYS);
    if (!numPolys)
        return;

    Vector3 actualLocalEnd = localEnd;

    // If full path was not found, clamp end point to the end polygon
    if (pathData_->polys_[numPolys - 1] != endRef)
        navMeshQuery_->closestPointOnPoly(pathData_->polys_[numPolys - 1], &localEnd.x_, &actualLocalEnd.x_, nullptr);

    navMeshQuery_->findStraightPath(&localStart.x_, &actualLocalEnd.x_, pathData_->polys_, numPolys,
        &pathData_->pathPoints_[0].x_, pathData_->pathFlags_, pathData_->pathPolys_, &numPathPoints, MAX_POLYS);

    // Transform path result back to world space
    for (int i{ 0 }; i < numPathPoints; ++i)
    {
        NavigationPathPoint pt;
        pt.position_ = transform * pathData_->pathPoints_[i];
        pt.flag_ = (NavigationPathPointFlag)pathData_->pathFlags_[i];

        // Walk through all NavAreas and find nearest
        unsigned nearestNavAreaID = 0;       // 0 is the default nav area ID
        float nearestDistance = M_LARGE_VALUE;
        for (unsigned j{ 0 }; j < areas_.Size(); j++)
        {
            NavArea* area = areas_[j].Get();
            if (area && area->IsEnabledEffective())
            {
                BoundingBox bb = area->GetWorldBoundingBox();
                if (bb.IsInside(pt.position_) == INSIDE)
                {
                    Vector3 areaWorldCenter = area->GetNode()->GetWorldPosition();
                    float distance = (areaWorldCenter - pt.position_).LengthSquared();
                    if (distance < nearestDistance)
                    {
                        nearestDistance = distance;
                        nearestNavAreaID = area->GetAreaID();
                    }
                }
            }
        }
        pt.areaID_ = (unsigned char)nearestNavAreaID;

        dest.Push(pt);
    }
}

Vector3 NavigationMesh::GetRandomPoint(const dtQueryFilter* filter, dtPolyRef* randomRef)
{
    if (!InitializeQuery())
        return Vector3::ZERO;

    dtPolyRef polyRef;
    Vector3 point(Vector3::ZERO);

    navMeshQuery_->findRandomPoint(filter ? filter : queryFilter_.Get(), Random, randomRef ? randomRef : &polyRef, &point.x_);

    return node_->GetWorldTransform() * point;
}

Vector3 NavigationMesh::GetRandomPointInCircle(const Vector3& center, float radius, const Vector3& extents,
    const dtQueryFilter* filter, dtPolyRef* randomRef)
{
    if (randomRef)
        *randomRef = 0;

    if (!InitializeQuery())
        return center;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();
    Vector3 localCenter = inverse * center;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.Get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localCenter.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return center;

    dtPolyRef polyRef;
    if (!randomRef)
        randomRef = &polyRef;
    Vector3 point(localCenter);

    navMeshQuery_->findRandomPointAroundCircle(startRef, &localCenter.x_, radius, queryFilter, Random, randomRef, &point.x_);

    return transform * point;
}

float NavigationMesh::GetDistanceToWall(const Vector3& point, float radius, const Vector3& extents, const dtQueryFilter* filter,
    Vector3* hitPos, Vector3* hitNormal)
{
    if (hitPos)
        *hitPos = Vector3::ZERO;
    if (hitNormal)
        *hitNormal = Vector3::DOWN;

    if (!InitializeQuery())
        return radius;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();
    Vector3 localPoint = inverse * point;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.Get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localPoint.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return radius;

    float hitDist = radius;
    Vector3 pos;
    if (!hitPos)
        hitPos = &pos;
    Vector3 normal;
    if (!hitNormal)
        hitNormal = &normal;

    navMeshQuery_->findDistanceToWall(startRef, &localPoint.x_, radius, queryFilter, &hitDist, &hitPos->x_, &hitNormal->x_);
    return hitDist;
}

Vector3 NavigationMesh::Raycast(const Vector3& start, const Vector3& end, const Vector3& extents, const dtQueryFilter* filter,
    Vector3* hitNormal)
{
    if (hitNormal)
        *hitNormal = Vector3::DOWN;

    if (!InitializeQuery())
        return end;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.Get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return end;

    Vector3 normal;
    if (!hitNormal)
        hitNormal = &normal;
    float t;
    int numPolys;

    navMeshQuery_->raycast(startRef, &localStart.x_, &localEnd.x_, queryFilter, &t, &hitNormal->x_, pathData_->polys_, &numPolys,
        MAX_POLYS);
    if (t == FLT_MAX)
        t = 1.0f;

    return start.Lerp(end, t);
}

void NavigationMesh::DrawDebugGeometry(bool depthTest)
{
    Scene* scene = GetScene();
    if (scene)
    {
        auto* debug = scene->GetComponent<DebugRenderer>();
        if (debug)
            DrawDebugGeometry(debug, depthTest);
    }
}

void NavigationMesh::SetAreaCost(unsigned areaID, float cost)
{
    if (queryFilter_)
        queryFilter_->setAreaCost((int)areaID, cost);
}

BoundingBox NavigationMesh::GetWorldBoundingBox() const
{
    return node_ ? boundingBox_.Transformed(node_->GetWorldTransform()) : boundingBox_;
}

float NavigationMesh::GetAreaCost(unsigned areaID) const
{
    if (queryFilter_)
        return queryFilter_->getAreaCost((int)areaID);
    return 1.0f;
}

void NavigationMesh::SetNavigationDataAttr(const PODVector<unsigned char>& value)
{
    ReleaseNavigationMesh();

    if (value.IsEmpty())
        return;

    MemoryBuffer buffer(value);

    boundingBox_ = buffer.ReadBoundingBox();
    numTilesX_ = buffer.ReadInt();
    numTilesZ_ = buffer.ReadInt();

    dtNavMeshParams params;     // NOLINT(hicpp-member-init)
    rcVcopy(params.orig, &boundingBox_.min_.x_);
    params.tileWidth = buffer.ReadFloat();
    params.tileHeight = buffer.ReadFloat();
    params.maxTiles = buffer.ReadInt();
    params.maxPolys = buffer.ReadInt();

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        DRY_LOGERROR("Could not allocate navigation mesh");
        return;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        DRY_LOGERROR("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return;
    }

    unsigned numTiles = 0;

    while (!buffer.IsEof())
    {
        if (ReadTile(buffer, true))
            ++numTiles;
        else
            return;
    }

    DRY_LOGDEBUG("Created navigation mesh with " + String(numTiles) + " tiles from serialized data");
    // \todo Shall we send E_NAVIGATION_MESH_REBUILT here?
}

PODVector<unsigned char> NavigationMesh::GetNavigationDataAttr() const
{
    VectorBuffer ret;

    if (navMesh_)
    {
        ret.WriteBoundingBox(boundingBox_);
        ret.WriteInt(numTilesX_);
        ret.WriteInt(numTilesZ_);

        const dtNavMeshParams* params = navMesh_->getParams();
        ret.WriteFloat(params->tileWidth);
        ret.WriteFloat(params->tileHeight);
        ret.WriteInt(params->maxTiles);
        ret.WriteInt(params->maxPolys);

        const dtNavMesh* navMesh = navMesh_;

        for (int z{ 0 }; z < numTilesZ_; ++z)
            for (int x{ 0 }; x < numTilesX_; ++x)
                WriteTile(ret, x, z);
    }

    return ret.GetBuffer();
}

void NavigationMesh::CollectGeometries(Vector<NavigationGeometryInfo>& geometryList)
{
    DRY_PROFILE(CollectNavigationGeometry);

    // Get Navigable components from child nodes, not from whole scene. This makes it possible to partition
    // the scene into several navigation meshes
    PODVector<Navigable*> navigables;
    node_->GetComponents<Navigable>(navigables, true);

    HashSet<Node*> processedNodes;
    for (unsigned i{ 0 }; i < navigables.Size(); ++i)
    {
        if (navigables[i]->IsEnabledEffective())
            CollectGeometries(geometryList, navigables[i]->GetNode(), processedNodes, navigables[i]->IsRecursive());
    }

    // Get offmesh connections
    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();
    PODVector<OffMeshConnection*> connections;
    node_->GetComponents<OffMeshConnection>(connections, true);

    for (unsigned i{ 0 }; i < connections.Size(); ++i)
    {
        OffMeshConnection* connection = connections[i];
        if (connection->IsEnabledEffective() && connection->GetEndPoint())
        {
            const Matrix3x4& transform = connection->GetNode()->GetWorldTransform();

            NavigationGeometryInfo info;
            info.component_ = connection;
            info.boundingBox_ = BoundingBox(Sphere(transform.Translation(), connection->GetRadius())).Transformed(inverse);

            geometryList.Push(info);
        }
    }

    // Get nav area volumes
    PODVector<NavArea*> navAreas;
    node_->GetComponents<NavArea>(navAreas, true);
    areas_.Clear();
    for (unsigned i{ 0 }; i < navAreas.Size(); ++i)
    {
        NavArea* area = navAreas[i];
        if (area->IsEnabledEffective())
        {
            NavigationGeometryInfo info;
            info.component_ = area;
            info.boundingBox_ = area->GetWorldBoundingBox();
            geometryList.Push(info);
            areas_.Push(WeakPtr<NavArea>(area));
        }
    }
}

void NavigationMesh::CollectGeometries(Vector<NavigationGeometryInfo>& geometryList, Node* node, HashSet<Node*>& processedNodes,
    bool recursive)
{
    // Make sure nodes are not included twice
    if (processedNodes.Contains(node))
        return;
    // Exclude obstacles and crowd agents from consideration
    if (node->HasComponent<Obstacle>() || node->HasComponent<CrowdAgent>())
        return;
    processedNodes.Insert(node);

    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();

#ifdef DRY_PHYSICS
    // Prefer compatible physics collision shapes (triangle mesh, convex hull, box) if found.
    // Then fallback to visible geometry
    PODVector<CollisionShape*> collisionShapes;
    node->GetComponents<CollisionShape>(collisionShapes);
    bool collisionShapeFound = false;

    for (unsigned i{ 0 }; i < collisionShapes.Size(); ++i)
    {
        CollisionShape* shape = collisionShapes[i];
        if (!shape->IsEnabledEffective())
            continue;

        ShapeType type = shape->GetShapeType();
        if ((type == SHAPE_BOX || type == SHAPE_TRIANGLEMESH || type == SHAPE_CONVEXHULL) && shape->GetCollisionShape())
        {
            Matrix3x4 shapeTransform(shape->GetPosition(), shape->GetRotation(), shape->GetSize());

            NavigationGeometryInfo info;
            info.component_ = shape;
            info.transform_ = inverse * node->GetWorldTransform() * shapeTransform;
            info.boundingBox_ = shape->GetWorldBoundingBox().Transformed(inverse);

            geometryList.Push(info);
            collisionShapeFound = true;
        }
    }
    if (!collisionShapeFound)
#endif
    {
        PODVector<Drawable*> drawables;
        node->GetDerivedComponents<Drawable>(drawables);

        for (unsigned i{ 0 }; i < drawables.Size(); ++i)
        {
            /// \todo Evaluate whether should handle other types. Now StaticModel & TerrainPatch are supported, others skipped
            Drawable* drawable = drawables[i];
            if (!drawable->IsEnabledEffective())
                continue;

            NavigationGeometryInfo info;

            if (drawable->GetType() == StaticModel::GetTypeStatic())
                info.lodLevel_ = static_cast<StaticModel*>(drawable)->GetOcclusionLodLevel();
            else if (drawable->GetType() == TerrainPatch::GetTypeStatic())
                info.lodLevel_ = 0;
            else
                continue;

            info.component_ = drawable;
            info.transform_ = inverse * node->GetWorldTransform();
            info.boundingBox_ = drawable->GetWorldBoundingBox().Transformed(inverse);

            geometryList.Push(info);
        }
    }

    if (recursive)
    {
        const Vector<SharedPtr<Node> >& children = node->GetChildren();
        for (unsigned i{ 0 }; i < children.Size(); ++i)
            CollectGeometries(geometryList, children[i], processedNodes, recursive);
    }
}

void NavigationMesh::GetTileGeometry(NavBuildData* build, Vector<NavigationGeometryInfo>& geometryList, BoundingBox& box)
{
    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();

    for (unsigned i{ 0 }; i < geometryList.Size(); ++i)
    {
        if (box.IsInsideFast(geometryList[i].boundingBox_) != OUTSIDE)
        {
            const Matrix3x4& transform = geometryList[i].transform_;

            if (geometryList[i].component_->GetType() == OffMeshConnection::GetTypeStatic())
            {
                auto* connection = static_cast<OffMeshConnection*>(geometryList[i].component_);
                Vector3 start = inverse * connection->GetNode()->GetWorldPosition();
                Vector3 end = inverse * connection->GetEndPoint()->GetWorldPosition();

                build->offMeshVertices_.Push(start);
                build->offMeshVertices_.Push(end);
                build->offMeshRadii_.Push(connection->GetRadius());
                build->offMeshFlags_.Push((unsigned short)connection->GetMask());
                build->offMeshAreas_.Push((unsigned char)connection->GetAreaID());
                build->offMeshDir_.Push((unsigned char)(connection->IsBidirectional() ? DT_OFFMESH_CON_BIDIR : 0));
                continue;
            }
            else if (geometryList[i].component_->GetType() == NavArea::GetTypeStatic())
            {
                auto* area = static_cast<NavArea*>(geometryList[i].component_);
                NavAreaStub stub;
                stub.areaID_ = (unsigned char)area->GetAreaID();
                stub.bounds_ = area->GetWorldBoundingBox();
                build->navAreas_.Push(stub);
                continue;
            }

#ifdef DRY_PHYSICS
            auto* shape = dynamic_cast<CollisionShape*>(geometryList[i].component_);
            if (shape)
            {
                switch (shape->GetShapeType())
                {
                case SHAPE_TRIANGLEMESH:
                    {
                        Model* model = shape->GetModel();
                        if (!model)
                            continue;

                        unsigned lodLevel = shape->GetLodLevel();
                        for (unsigned j{ 0 }; j < model->GetNumGeometries(); ++j)
                            AddTriMeshGeometry(build, model->GetGeometry(j, lodLevel), transform);
                    }
                    break;

                case SHAPE_CONVEXHULL:
                    {
                        auto* data = static_cast<ConvexData*>(shape->GetGeometryData());
                        if (!data)
                            continue;

                        unsigned numVertices = data->vertexCount_;
                        unsigned numIndices = data->indexCount_;
                        unsigned destVertexStart = build->vertices_.Size();

                        for (unsigned j{ 0 }; j < numVertices; ++j)
                            build->vertices_.Push(transform * data->vertexData_[j]);

                        for (unsigned j{ 0 }; j < numIndices; ++j)
                            build->indices_.Push(data->indexData_[j] + destVertexStart);
                    }
                    break;

                case SHAPE_BOX:
                    {
                        unsigned destVertexStart = build->vertices_.Size();

                        build->vertices_.Push(transform * Vector3(-0.5f, 0.5f, -0.5f));
                        build->vertices_.Push(transform * Vector3(0.5f, 0.5f, -0.5f));
                        build->vertices_.Push(transform * Vector3(0.5f, -0.5f, -0.5f));
                        build->vertices_.Push(transform * Vector3(-0.5f, -0.5f, -0.5f));
                        build->vertices_.Push(transform * Vector3(-0.5f, 0.5f, 0.5f));
                        build->vertices_.Push(transform * Vector3(0.5f, 0.5f, 0.5f));
                        build->vertices_.Push(transform * Vector3(0.5f, -0.5f, 0.5f));
                        build->vertices_.Push(transform * Vector3(-0.5f, -0.5f, 0.5f));

                        const unsigned indices[] = {
                            0, 1, 2, 0, 2, 3, 1, 5, 6, 1, 6, 2, 4, 5, 1, 4, 1, 0, 5, 4, 7, 5, 7, 6,
                            4, 0, 3, 4, 3, 7, 1, 0, 4, 1, 4, 5
                        };

                        for (unsigned index : indices)
                            build->indices_.Push(index + destVertexStart);
                    }
                    break;

                default:
                    break;
                }

                continue;
            }
#endif
            auto* drawable = dynamic_cast<Drawable*>(geometryList[i].component_);
            if (drawable)
            {
                const Vector<SourceBatch>& batches = drawable->GetBatches();

                for (unsigned j{ 0 }; j < batches.Size(); ++j)
                    AddTriMeshGeometry(build, drawable->GetLodGeometry(j, geometryList[i].lodLevel_), transform);
            }
        }
    }
}

void NavigationMesh::AddTriMeshGeometry(NavBuildData* build, Geometry* geometry, const Matrix3x4& transform)
{
    if (!geometry)
        return;

    const unsigned char* vertexData;
    const unsigned char* indexData;
    unsigned vertexSize;
    unsigned indexSize;
    const PODVector<VertexElement>* elements;

    geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elements);
    if (!vertexData || !indexData || !elements || VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_POSITION) != 0)
        return;

    unsigned srcIndexStart = geometry->GetIndexStart();
    unsigned srcIndexCount = geometry->GetIndexCount();
    unsigned srcVertexStart = geometry->GetVertexStart();
    unsigned srcVertexCount = geometry->GetVertexCount();

    if (!srcIndexCount)
        return;

    unsigned destVertexStart = build->vertices_.Size();

    for (unsigned k{ srcVertexStart }; k < srcVertexStart + srcVertexCount; ++k)
    {
        Vector3 vertex = transform * *((const Vector3*)(&vertexData[k * vertexSize]));
        build->vertices_.Push(vertex);
    }

    // Copy remapped indices
    if (indexSize == sizeof(unsigned short))
    {
        const unsigned short* indices = ((const unsigned short*)indexData) + srcIndexStart;
        const unsigned short* indicesEnd = indices + srcIndexCount;

        while (indices < indicesEnd)
        {
            build->indices_.Push(*indices - srcVertexStart + destVertexStart);
            ++indices;
        }
    }
    else
    {
        const unsigned* indices = ((const unsigned*)indexData) + srcIndexStart;
        const unsigned* indicesEnd = indices + srcIndexCount;

        while (indices < indicesEnd)
        {
            build->indices_.Push(*indices - srcVertexStart + destVertexStart);
            ++indices;
        }
    }
}

void NavigationMesh::WriteTile(Serializer& dest, int x, int z) const
{
    const dtNavMesh* navMesh = navMesh_;
    const dtMeshTile* tile = navMesh->getTileAt(x, z, 0);
    if (!tile)
        return;

    dest.WriteInt(x);
    dest.WriteInt(z);
    dest.WriteUInt(navMesh->getTileRef(tile));
    dest.WriteUInt((unsigned)tile->dataSize);
    dest.Write(tile->data, (unsigned)tile->dataSize);
}

bool NavigationMesh::ReadTile(Deserializer& source, bool silent)
{
    const int x = source.ReadInt();
    const int z = source.ReadInt();
    /*dtTileRef tileRef =*/ source.ReadUInt();
    unsigned navDataSize = source.ReadUInt();

    auto* navData = (unsigned char*)dtAlloc(navDataSize, DT_ALLOC_PERM);
    if (!navData)
    {
        DRY_LOGERROR("Could not allocate data for navigation mesh tile");
        return false;
    }

    source.Read(navData, navDataSize);
    if (dtStatusFailed(navMesh_->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr)))
    {
        DRY_LOGERROR("Failed to add navigation mesh tile");
        dtFree(navData);
        return false;
    }

    // Send event
    if (!silent)
    {
        using namespace NavigationTileAdded;
        VariantMap& eventData = GetContext()->GetEventDataMap();
        eventData[P_NODE] = GetNode();
        eventData[P_MESH] = this;
        eventData[P_TILE] = IntVector2(x, z);
        SendEvent(E_NAVIGATION_TILE_ADDED, eventData);
    }
    return true;
}

bool NavigationMesh::BuildTile(Vector<NavigationGeometryInfo>& geometryList, int x, int z)
{
    DRY_PROFILE(BuildNavigationMeshTile);

    // Remove previous tile (if any)
    navMesh_->removeTile(navMesh_->getTileRefAt(x, z, 0), nullptr, nullptr);

    const BoundingBox tileBoundingBox = GetTileBoundingBox(IntVector2(x, z));

    SimpleNavBuildData build;

    rcConfig cfg;       // NOLINT(hicpp-member-init)
    memset(&cfg, 0, sizeof cfg);
    cfg.cs = cellSize_;
    cfg.ch = cellHeight_;
    cfg.walkableSlopeAngle = agentMaxSlope_;
    cfg.walkableHeight = CeilToInt(agentHeight_ / cfg.ch);
    cfg.walkableClimb = FloorToInt(agentMaxClimb_ / cfg.ch);
    cfg.walkableRadius = CeilToInt(agentRadius_ / cfg.cs);
    cfg.maxEdgeLen = (int)(edgeMaxLength_ / cellSize_);
    cfg.maxSimplificationError = edgeMaxError_;
    cfg.minRegionArea = (int)sqrtf(regionMinSize_);
    cfg.mergeRegionArea = (int)sqrtf(regionMergeSize_);
    cfg.maxVertsPerPoly = 6;
    cfg.tileSize = tileSize_;
    cfg.borderSize = cfg.walkableRadius + 3; // Add padding
    cfg.width = cfg.tileSize + cfg.borderSize * 2;
    cfg.height = cfg.tileSize + cfg.borderSize * 2;
    cfg.detailSampleDist = detailSampleDistance_ < 0.9f ? 0.0f : cellSize_ * detailSampleDistance_;
    cfg.detailSampleMaxError = cellHeight_ * detailSampleMaxError_;

    rcVcopy(cfg.bmin, &tileBoundingBox.min_.x_);
    rcVcopy(cfg.bmax, &tileBoundingBox.max_.x_);
    cfg.bmin[0] -= cfg.borderSize * cfg.cs;
    cfg.bmin[2] -= cfg.borderSize * cfg.cs;
    cfg.bmax[0] += cfg.borderSize * cfg.cs;
    cfg.bmax[2] += cfg.borderSize * cfg.cs;

    BoundingBox expandedBox(*reinterpret_cast<Vector3*>(cfg.bmin), *reinterpret_cast<Vector3*>(cfg.bmax));
    GetTileGeometry(&build, geometryList, expandedBox);

    if (build.vertices_.IsEmpty() || build.indices_.IsEmpty())
        return true; // Nothing to do

    build.heightField_ = rcAllocHeightfield();
    if (!build.heightField_)
    {
        DRY_LOGERROR("Could not allocate heightfield");
        return false;
    }

    if (!rcCreateHeightfield(build.ctx_, *build.heightField_, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs,
        cfg.ch))
    {
        DRY_LOGERROR("Could not create heightfield");
        return false;
    }

    unsigned numTriangles = build.indices_.Size() / 3;
    SharedArrayPtr<unsigned char> triAreas(new unsigned char[numTriangles]);
    memset(triAreas.Get(), 0, numTriangles);

    rcMarkWalkableTriangles(build.ctx_, cfg.walkableSlopeAngle, &build.vertices_[0].x_, build.vertices_.Size(),
        &build.indices_[0], numTriangles, triAreas.Get());
    rcRasterizeTriangles(build.ctx_, &build.vertices_[0].x_, build.vertices_.Size(), &build.indices_[0],
        triAreas.Get(), numTriangles, *build.heightField_, cfg.walkableClimb);
    rcFilterLowHangingWalkableObstacles(build.ctx_, cfg.walkableClimb, *build.heightField_);

    rcFilterWalkableLowHeightSpans(build.ctx_, cfg.walkableHeight, *build.heightField_);
    rcFilterLedgeSpans(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_);

    build.compactHeightField_ = rcAllocCompactHeightfield();
    if (!build.compactHeightField_)
    {
        DRY_LOGERROR("Could not allocate create compact heightfield");
        return false;
    }
    if (!rcBuildCompactHeightfield(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_,
        *build.compactHeightField_))
    {
        DRY_LOGERROR("Could not build compact heightfield");
        return false;
    }
    if (!rcErodeWalkableArea(build.ctx_, cfg.walkableRadius, *build.compactHeightField_))
    {
        DRY_LOGERROR("Could not erode compact heightfield");
        return false;
    }

    // Mark area volumes
    for (unsigned i{ 0 }; i < build.navAreas_.Size(); ++i)
        rcMarkBoxArea(build.ctx_, &build.navAreas_[i].bounds_.min_.x_, &build.navAreas_[i].bounds_.max_.x_,
            build.navAreas_[i].areaID_, *build.compactHeightField_);

    if (this->partitionType_ == NAVMESH_PARTITION_WATERSHED)
    {
        if (!rcBuildDistanceField(build.ctx_, *build.compactHeightField_))
        {
            DRY_LOGERROR("Could not build distance field");
            return false;
        }
        if (!rcBuildRegions(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea,
            cfg.mergeRegionArea))
        {
            DRY_LOGERROR("Could not build regions");
            return false;
        }
    }
    else
    {
        if (!rcBuildRegionsMonotone(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
        {
            DRY_LOGERROR("Could not build monotone regions");
            return false;
        }
    }

    build.contourSet_ = rcAllocContourSet();
    if (!build.contourSet_)
    {
        DRY_LOGERROR("Could not allocate contour set");
        return false;
    }
    if (!rcBuildContours(build.ctx_, *build.compactHeightField_, cfg.maxSimplificationError, cfg.maxEdgeLen,
        *build.contourSet_))
    {
        DRY_LOGERROR("Could not create contours");
        return false;
    }

    build.polyMesh_ = rcAllocPolyMesh();
    if (!build.polyMesh_)
    {
        DRY_LOGERROR("Could not allocate poly mesh");
        return false;
    }
    if (!rcBuildPolyMesh(build.ctx_, *build.contourSet_, cfg.maxVertsPerPoly, *build.polyMesh_))
    {
        DRY_LOGERROR("Could not triangulate contours");
        return false;
    }

    build.polyMeshDetail_ = rcAllocPolyMeshDetail();
    if (!build.polyMeshDetail_)
    {
        DRY_LOGERROR("Could not allocate detail mesh");
        return false;
    }
    if (!rcBuildPolyMeshDetail(build.ctx_, *build.polyMesh_, *build.compactHeightField_, cfg.detailSampleDist,
        cfg.detailSampleMaxError, *build.polyMeshDetail_))
    {
        DRY_LOGERROR("Could not build detail mesh");
        return false;
    }

    // Set polygon flags
    /// \todo Assignment of flags from navigation areas?
    for (int i{ 0 }; i < build.polyMesh_->npolys; ++i)
    {
        if (build.polyMesh_->areas[i] != RC_NULL_AREA)
            build.polyMesh_->flags[i] = 0x1;
    }

    unsigned char* navData = nullptr;
    int navDataSize = 0;

    dtNavMeshCreateParams params;       // NOLINT(hicpp-member-init)
    memset(&params, 0, sizeof params);
    params.verts = build.polyMesh_->verts;
    params.vertCount = build.polyMesh_->nverts;
    params.polys = build.polyMesh_->polys;
    params.polyAreas = build.polyMesh_->areas;
    params.polyFlags = build.polyMesh_->flags;
    params.polyCount = build.polyMesh_->npolys;
    params.nvp = build.polyMesh_->nvp;
    params.detailMeshes = build.polyMeshDetail_->meshes;
    params.detailVerts = build.polyMeshDetail_->verts;
    params.detailVertsCount = build.polyMeshDetail_->nverts;
    params.detailTris = build.polyMeshDetail_->tris;
    params.detailTriCount = build.polyMeshDetail_->ntris;
    params.walkableHeight = agentHeight_;
    params.walkableRadius = agentRadius_;
    params.walkableClimb = agentMaxClimb_;
    params.tileX = x;
    params.tileY = z;
    rcVcopy(params.bmin, build.polyMesh_->bmin);
    rcVcopy(params.bmax, build.polyMesh_->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;

    // Add off-mesh connections if have them
    if (build.offMeshRadii_.Size())
    {
        params.offMeshConCount = build.offMeshRadii_.Size();
        params.offMeshConVerts = &build.offMeshVertices_[0].x_;
        params.offMeshConRad = &build.offMeshRadii_[0];
        params.offMeshConFlags = &build.offMeshFlags_[0];
        params.offMeshConAreas = &build.offMeshAreas_[0];
        params.offMeshConDir = &build.offMeshDir_[0];
    }

    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        DRY_LOGERROR("Could not build navigation mesh tile data");
        return false;
    }

    if (dtStatusFailed(navMesh_->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr)))
    {
        DRY_LOGERROR("Failed to add navigation mesh tile");
        dtFree(navData);
        return false;
    }

    // Send a notification of the rebuild of this tile to anyone interested
    {
        using namespace NavigationAreaRebuilt;
        VariantMap& eventData = GetContext()->GetEventDataMap();
        eventData[P_NODE] = GetNode();
        eventData[P_MESH] = this;
        eventData[P_BOUNDSMIN] = Variant(tileBoundingBox.min_);
        eventData[P_BOUNDSMAX] = Variant(tileBoundingBox.max_);
        SendEvent(E_NAVIGATION_AREA_REBUILT, eventData);
    }
    return true;
}

unsigned NavigationMesh::BuildTiles(Vector<NavigationGeometryInfo>& geometryList, const IntVector2& from, const IntVector2& to)
{
    unsigned numTiles = 0;

    for (int z = from.y_; z <= to.y_; ++z)
    {
        for (int x = from.x_; x <= to.x_; ++x)
        {
            if (BuildTile(geometryList, x, z))
                ++numTiles;
        }
    }
    return numTiles;
}

bool NavigationMesh::InitializeQuery()
{
    if (!navMesh_ || !node_)
        return false;

    if (navMeshQuery_)
        return true;

    navMeshQuery_ = dtAllocNavMeshQuery();
    if (!navMeshQuery_)
    {
        DRY_LOGERROR("Could not create navigation mesh query");
        return false;
    }

    if (dtStatusFailed(navMeshQuery_->init(navMesh_, MAX_POLYS)))
    {
        DRY_LOGERROR("Could not init navigation mesh query");
        return false;
    }

    return true;
}

void NavigationMesh::ReleaseNavigationMesh()
{
    dtFreeNavMesh(navMesh_);
    navMesh_ = nullptr;

    dtFreeNavMeshQuery(navMeshQuery_);
    navMeshQuery_ = nullptr;

    numTilesX_ = 0;
    numTilesZ_ = 0;
    boundingBox_.Clear();
}

void NavigationMesh::SetPartitionType(NavmeshPartitionType partitionType)
{
    partitionType_ = partitionType;
    MarkNetworkUpdate();
}

void RegisterNavigationLibrary(Context* context)
{
    Navigable::RegisterObject(context);
    NavigationMesh::RegisterObject(context);
    OffMeshConnection::RegisterObject(context);
    CrowdAgent::RegisterObject(context);
    CrowdManager::RegisterObject(context);
    DynamicNavigationMesh::RegisterObject(context);
    Obstacle::RegisterObject(context);
    NavArea::RegisterObject(context);
}

}
