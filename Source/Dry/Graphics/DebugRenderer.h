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

#include "../Math/Color.h"
#include "../Math/Frustum.h"
#include "../Scene/Component.h"

namespace Dry
{

class BoundingBox;
class Camera;
class Polyhedron;
class Drawable;
class Light;
class Matrix3x4;
class Renderer;
class Skeleton;
class Sphere;
class VertexBuffer;

/// Debug rendering line.
struct DebugLine
{
    /// Construct undefined.
    DebugLine() = default;

    /// Construct with start and end positions and color.
    DebugLine(const Vector3& start, const Vector3& end, unsigned color) :
        start_(start),
        end_(end),
        color_(color)
    {
    }

    /// Start position.
    Vector3 start_;
    /// End position.
    Vector3 end_;
    /// Color.
    unsigned color_{};
};

/// Debug render triangle.
struct DebugTriangle
{
    /// Construct undefined.
    DebugTriangle() = default;

    /// Construct with start and end positions and color.
    DebugTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, unsigned color) :
        v1_(v1),
        v2_(v2),
        v3_(v3),
        color_(color)
    {
    }

    /// Vertex a.
    Vector3 v1_;
    /// Vertex b.
    Vector3 v2_;
    /// Vertex c.
    Vector3 v3_;
    /// Color.
    unsigned color_{};
};

/// Debug geometry rendering component. Should be added only to the root scene node.
class DRY_API DebugRenderer : public Component
{
    DRY_OBJECT(DebugRenderer, Component);

public:
    /// Construct.
    explicit DebugRenderer(Context* context);
    /// Destruct.
    ~DebugRenderer() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set line antialiasing on/off. Default false.
    void SetLineAntiAlias(bool enable);
    /// Set the camera viewpoint. Call before rendering, or before adding geometry if you want to use culling.
    void SetView(Camera* camera);
    /// Add a line.
    void AddLine(const Vector3& start, const Vector3& end, const Color& color, bool depthTest = true);
    /// Add a line with color already converted to unsigned.
    void AddLine(const Vector3& start, const Vector3& end, unsigned color, bool depthTest = true);
    /// Add a solid triangle.
    void AddTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Color& color, bool depthTest = true);
    /// Add a solid triangle with color already converted to unsigned.
    void AddTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, unsigned color, bool depthTest = true);
    /// Add a solid quadrangular polygon.
    void AddPolygon(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& v4, const Color& color, bool depthTest = true);
    /// Add a solid quadrangular polygon with color already converted to unsigned.
    void AddPolygon(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& v4, unsigned color, bool depthTest = true);
    /// Add a scene node represented as its coordinate axes.
    void AddNode(Node* node, float scale = 1.0f, bool depthTest = true);
    /// Add a bounding box.
    void AddBoundingBox(const BoundingBox& box, const Color& color, bool depthTest = true, bool solid = false);
    /// Add a bounding box with transform.
    void AddBoundingBox(const BoundingBox& box, const Matrix3x4& transform, const Color& color, bool depthTest = true, bool solid = false);
    /// Add a frustum.
    void AddFrustum(const Frustum& frustum, const Color& color, bool depthTest = true);
    /// Add a polyhedron.
    void AddPolyhedron(const Polyhedron& poly, const Color& color, bool depthTest = true);
    /// Add a sphere.
    void AddSphere(const Sphere& sphere, const Color& color, bool depthTest = true);
    /// Add a sphere sector. Angle ranges from 0 to 360. Identity Quaternion yields the filled portion of the sector upwards.
    void AddSphereSector(const Sphere& sphere, const Quaternion& rotation, float angle,
        bool drawLines, const Color& color, bool depthTest = true);
    /// Add a cylinder
    void AddCylinder(const Vector3& position, float radius, float height, const Color& color, bool depthTest = true);
    /// Add a skeleton.
    void AddSkeleton(const Skeleton& skeleton, const Color& color, bool depthTest = true);
    /// Add a triangle mesh.
    void AddTriangleMesh(const void* vertexData, unsigned vertexSize, const void* indexData,
        unsigned indexSize, unsigned indexStart, unsigned indexCount, const Matrix3x4& transform, const Color& color, bool depthTest = true);
    /// Add a triangle mesh.
    void AddTriangleMesh(const void* vertexData, unsigned vertexSize, unsigned vertexStart, const void* indexData,
        unsigned indexSize, unsigned indexStart, unsigned indexCount, const Matrix3x4& transform, const Color& color, bool depthTest = true);
    /// Add a circle.
    void AddCircle(const Vector3& center, const Vector3& normal, float radius, const Color& color, int steps = 64, bool depthTest = true);
    /// Add a cross.
    void AddCross(const Vector3& center, float size, const Color& color, bool depthTest = true);
    /// Add a quad on the XZ plane.
    void AddQuad(const Vector3& center, float width, float height, const Color& color, bool depthTest = true);

    /// Update vertex buffer and render all debug lines. The viewport and rendertarget should be set before.
    void Render();

    /// Return whether line antialiasing is enabled.
    bool GetLineAntiAlias() const { return lineAntiAlias_; }

    /// Return the view transform.
    const Matrix3x4& GetView() const { return view_; }

    /// Return the projection transform.
    const Matrix4& GetProjection() const { return projection_; }

    /// Return the view frustum.
    const Frustum& GetFrustum() const { return frustum_; }

    /// Check whether a bounding box is inside the view frustum.
    bool IsInside(const BoundingBox& box) const;
    /// Return whether has something to render.
    bool HasContent() const;

private:
    /// Handle end of frame. Clear debug geometry.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);

    /// Lines rendered with depth test.
    PODVector<DebugLine> lines_;
    /// Lines rendered without depth test.
    PODVector<DebugLine> noDepthLines_;
    /// Triangles rendered with depth test.
    PODVector<DebugTriangle> triangles_;
    /// Triangles rendered without depth test.
    PODVector<DebugTriangle> noDepthTriangles_;
    /// View transform.
    Matrix3x4 view_;
    /// Projection transform.
    Matrix4 projection_;
    /// Projection transform in API-specific format.
    Matrix4 gpuProjection_;
    /// View frustum.
    Frustum frustum_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Line antialiasing flag.
    bool lineAntiAlias_;
};

}
