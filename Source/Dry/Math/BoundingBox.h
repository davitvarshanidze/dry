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

#include "../Math/Rect.h"
#include "../Math/Vector3.h"

#ifdef DRY_SSE
#include <xmmintrin.h>
#endif

namespace Dry
{

class Polyhedron;
class Frustum;
class Matrix3;
class Matrix4;
class Matrix3x4;
class Sphere;

/// Three-dimensional axis-aligned bounding box.
class DRY_API BoundingBox
{
public:
    /// Construct with zero size.
    BoundingBox() noexcept :
        min_(M_INFINITY, M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
    }

    /// Copy-construct from another bounding box.
    BoundingBox(const BoundingBox& box) noexcept :
        min_(box.min_),
        max_(box.max_)
    {
    }

    /// Construct from a rect, with the Z dimension left zero.
    explicit BoundingBox(const Rect& rect) noexcept :
        min_(Vector3(rect.min_, 0.0f)),
        max_(Vector3(rect.max_, 0.0f))
    {
    }

    /// Construct from minimum and maximum vectors.
    BoundingBox(const Vector3& min, const Vector3& max) noexcept :
        min_(min),
        max_(max)
    {
    }

    /// Construct from minimum and maximum floats (all dimensions same.)
    BoundingBox(float min, float max) noexcept :
        min_(Vector3(min, min, min)),
        max_(Vector3(max, max, max))
    {
    }

#ifdef DRY_SSE
    BoundingBox(__m128 min, __m128 max) noexcept
    {
        _mm_storeu_ps(&min_.x_, min);
        _mm_storeu_ps(&max_.x_, max);
    }
#endif

    /// Construct from an array of vertices.
    BoundingBox(const Vector3* vertices, unsigned count) :
        min_(M_INFINITY, M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(vertices, count);
    }

    /// Construct from a frustum.
    explicit BoundingBox(const Frustum& frustum) :
        min_(M_INFINITY, M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(frustum);
    }

    /// Construct from a polyhedron.
    explicit BoundingBox(const Polyhedron& poly) :
        min_(M_INFINITY, M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(poly);
    }

    /// Construct from a sphere.
    explicit BoundingBox(const Sphere& sphere) :
        min_(M_INFINITY, M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(sphere);
    }

    /// Assign from another bounding box.
    BoundingBox& operator =(const BoundingBox& rhs) noexcept
    {
        min_ = rhs.min_;
        max_ = rhs.max_;
        return *this;
    }

    /// Assign from a Rect, with the Z dimension left zero.
    BoundingBox& operator =(const Rect& rhs) noexcept
    {
        min_ = Vector3(rhs.min_, 0.0f);
        max_ = Vector3(rhs.max_, 0.0f);
        return *this;
    }

    /// Test for equality with another bounding box.
    bool operator ==(const BoundingBox& rhs) const { return (min_ == rhs.min_ && max_ == rhs.max_); }

    /// Test for inequality with another bounding box.
    bool operator !=(const BoundingBox& rhs) const { return (min_ != rhs.min_ || max_ != rhs.max_); }

    /// Define from another bounding box.
    void Define(const BoundingBox& box)
    {
        Define(box.min_, box.max_);
    }

    /// Define from a Rect.
    void Define(const Rect& rect)
    {
        Define(Vector3(rect.min_, 0.0f), Vector3(rect.max_, 0.0f));
    }

    /// Define from minimum and maximum vectors.
    void Define(const Vector3& min, const Vector3& max)
    {
        min_ = min;
        max_ = max;
    }

    /// Define from minimum and maximum floats (all dimensions same.)
    void Define(float min, float max)
    {
        min_ = Vector3(min, min, min);
        max_ = Vector3(max, max, max);
    }

    /// Define from a point.
    void Define(const Vector3& point)
    {
        min_ = max_ = point;
    }

    /// Merge a point.
    void Merge(const Vector3& point)
    {
#ifdef DRY_SSE
        __m128 vec = _mm_set_ps(1.f, point.z_, point.y_, point.x_);
        _mm_storeu_ps(&min_.x_, _mm_min_ps(_mm_loadu_ps(&min_.x_), vec));
        _mm_storeu_ps(&max_.x_, _mm_max_ps(_mm_loadu_ps(&max_.x_), vec));
#else
        if (point.x_ < min_.x_)
            min_.x_ = point.x_;
        if (point.y_ < min_.y_)
            min_.y_ = point.y_;
        if (point.z_ < min_.z_)
            min_.z_ = point.z_;
        if (point.x_ > max_.x_)
            max_.x_ = point.x_;
        if (point.y_ > max_.y_)
            max_.y_ = point.y_;
        if (point.z_ > max_.z_)
            max_.z_ = point.z_;
#endif
    }

    /// Merge another bounding box.
    void Merge(const BoundingBox& box)
    {
#ifdef DRY_SSE
        _mm_storeu_ps(&min_.x_, _mm_min_ps(_mm_loadu_ps(&min_.x_), _mm_loadu_ps(&box.min_.x_)));
        _mm_storeu_ps(&max_.x_, _mm_max_ps(_mm_loadu_ps(&max_.x_), _mm_loadu_ps(&box.max_.x_)));
#else
        if (box.min_.x_ < min_.x_)
            min_.x_ = box.min_.x_;
        if (box.min_.y_ < min_.y_)
            min_.y_ = box.min_.y_;
        if (box.min_.z_ < min_.z_)
            min_.z_ = box.min_.z_;
        if (box.max_.x_ > max_.x_)
            max_.x_ = box.max_.x_;
        if (box.max_.y_ > max_.y_)
            max_.y_ = box.max_.y_;
        if (box.max_.z_ > max_.z_)
            max_.z_ = box.max_.z_;
#endif
    }

    /// Define from an array of vertices.
    void Define(const Vector3* vertices, unsigned count);
    /// Define from a frustum.
    void Define(const Frustum& frustum);
    /// Define from a polyhedron.
    void Define(const Polyhedron& poly);
    /// Define from a sphere.
    void Define(const Sphere& sphere);
    /// Merge an array of vertices.
    void Merge(const Vector3* vertices, unsigned count);
    /// Merge a frustum.
    void Merge(const Frustum& frustum);
    /// Merge a polyhedron.
    void Merge(const Polyhedron& poly);
    /// Merge a sphere.
    void Merge(const Sphere& sphere);
    /// Clip with another bounding box. The box can become degenerate (undefined) as a result.
    void Clip(const BoundingBox& box);
    /// Transform with a 3x3 matrix.
    void Transform(const Matrix3& transform);
    /// Transform with a 3x4 matrix.
    void Transform(const Matrix3x4& transform);

    /// Clear to undefined state.
    void Clear()
    {
#ifdef DRY_SSE
        _mm_storeu_ps(&min_.x_, _mm_set1_ps(M_INFINITY));
        _mm_storeu_ps(&max_.x_, _mm_set1_ps(-M_INFINITY));
#else
        min_ = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
        max_ = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);
#endif
    }

    /// Return true if this bounding box is defined via a previous call to Define() or Merge().
    bool Defined() const
    {
        return min_.x_ != M_INFINITY;
    }

    /// Return center.
    Vector3 Center() const { return (max_ + min_) * 0.5f; }

    /// Return size.
    Vector3 Size() const { return max_ - min_; }

    /// Return half-size.
    Vector3 HalfSize() const { return (max_ - min_) * 0.5f; }

    /// Return transformed by a 3x3 matrix.
    BoundingBox Transformed(const Matrix3& transform) const;
    /// Return transformed by a 3x4 matrix.
    BoundingBox Transformed(const Matrix3x4& transform) const;
    /// Return projected by a 4x4 projection matrix.
    Rect Projected(const Matrix4& projection) const;
    /// Return distance to point.
    float DistanceToPoint(const Vector3& point) const;

    /// Test if a point is inside.
    Intersection IsInside(const Vector3& point) const
    {
        if (point.x_ < min_.x_ || point.x_ > max_.x_ || point.y_ < min_.y_ || point.y_ > max_.y_ ||
            point.z_ < min_.z_ || point.z_ > max_.z_)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Test if another bounding box is inside, outside or intersects.
    Intersection IsInside(const BoundingBox& box) const
    {
        if (box.max_.x_ < min_.x_ || box.min_.x_ > max_.x_ || box.max_.y_ < min_.y_ || box.min_.y_ > max_.y_ ||
            box.max_.z_ < min_.z_ || box.min_.z_ > max_.z_)
            return OUTSIDE;
        else if (box.min_.x_ < min_.x_ || box.max_.x_ > max_.x_ || box.min_.y_ < min_.y_ || box.max_.y_ > max_.y_ ||
                 box.min_.z_ < min_.z_ || box.max_.z_ > max_.z_)
            return INTERSECTS;
        else
            return INSIDE;
    }

    /// Test if another bounding box is (partially) inside or outside.
    Intersection IsInsideFast(const BoundingBox& box) const
    {
        if (box.max_.x_ < min_.x_ || box.min_.x_ > max_.x_ || box.max_.y_ < min_.y_ || box.min_.y_ > max_.y_ ||
            box.max_.z_ < min_.z_ || box.min_.z_ > max_.z_)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Test if a sphere is inside, outside or intersects.
    Intersection IsInside(const Sphere& sphere) const;
    /// Test if a sphere is (partially) inside or outside.
    Intersection IsInsideFast(const Sphere& sphere) const;

    /// Return as string.
    String ToString() const;

    /// Minimum vector.
    Vector3 min_;
    float dummyMin_{}; // This is never used, but exists to pad the min_ value to four floats.
    /// Maximum vector.
    Vector3 max_;
    float dummyMax_{}; // This is never used, but exists to pad the max_ value to four floats.
};

/// Three-dimensional bounding rectangle with integer values.
class DRY_API IntBoundingBox
{
public:
    /// Construct a zero rect.
    IntBoundingBox() noexcept :
        left_(0),
        bottom_(0),
        back_(0),
        top_(0),
        front_(0),
        right_(0)
    {
    }

    /// Construct from minimum and maximum vectors.
    IntBoundingBox(const IntVector3& min, const IntVector3& max) noexcept :
        left_(min.x_),
        bottom_(min.y_),
        back_(min.z_),
        right_(max.x_),
        top_(max.y_),
        front_(max.z_)
    {
    }

    /// Construct from coordinates.
    IntBoundingBox(int left, int bottom, int back, int right, int top, int front) noexcept :
        left_(left),
        bottom_(bottom),
        back_(back),
        right_(right),
        top_(top),
        front_(front)
    {
    }

    /// Construct from an int array.
    explicit IntBoundingBox(const int* data) noexcept :
        left_(data[0]),
        bottom_(data[1]),
        back_(data[2]),
        right_(data[3]),
        top_(data[4]),
        front_(data[5])
    {
    }

    /// Test for equality with another rect.
    bool operator ==(const IntBoundingBox& rhs) const
    {
        return left_  == rhs.left_  && bottom_ == rhs.bottom_ && back_  == rhs.back_ &&
               right_ == rhs.right_ && top_ == rhs.top_       && front_ == rhs.front_;
    }

    /// Test for inequality with another rect.
    bool operator !=(const IntBoundingBox& rhs) const
    {
        return left_  != rhs.left_  || bottom_ != rhs.bottom_ || back_  != rhs.back_ ||
               right_ != rhs.right_ || top_    != rhs.top_    || front_ != rhs.front_;
    }

    /// Add another rect to this one inplace.
    IntBoundingBox& operator +=(const IntBoundingBox& rhs)
    {
        left_ += rhs.left_;
        bottom_ += rhs.bottom_;
        back_ += rhs.back_;
        right_ += rhs.right_;
        top_ += rhs.top_;
        front_ += rhs.front_;
        return *this;
    }

    /// Subtract another rect from this one inplace.
    IntBoundingBox& operator -=(const IntBoundingBox& rhs)
    {
        left_ -= rhs.left_;
        bottom_ -= rhs.bottom_;
        back_ -= rhs.back_;
        right_ -= rhs.right_;
        top_ -= rhs.top_;
        front_ -= rhs.front_;
        return *this;
    }

    /// Divide by scalar inplace.
    IntBoundingBox& operator /=(float value)
    {
        left_ = static_cast<int>(left_ / value);
        bottom_ = static_cast<int>(bottom_ / value);
        back_ = static_cast<int>(back_ / value);
        right_ = static_cast<int>(right_ / value);
        top_ = static_cast<int>(top_ / value);
        front_ = static_cast<int>(front_ / value);
        return *this;
    }

    /// Multiply by scalar inplace.
    IntBoundingBox& operator *=(float value)
    {
        left_ = static_cast<int>(left_ * value);
        bottom_ = static_cast<int>(bottom_ * value);
        back_ = static_cast<int>(back_ * value);
        right_ = static_cast<int>(right_ * value);
        top_ = static_cast<int>(top_ * value);
        front_ = static_cast<int>(front_ * value);
        return *this;
    }

    /// Divide by scalar.
    IntBoundingBox operator /(float value) const
    {
        return {
            static_cast<int>(left_ / value), static_cast<int>(bottom_ / value), static_cast<int>(back_ / value),
            static_cast<int>(right_ / value), static_cast<int>(top_ / value), static_cast<int>(front_ / value)
        };
    }

    /// Multiply by scalar.
    IntBoundingBox operator *(float value) const
    {
        return {
            static_cast<int>(left_ * value), static_cast<int>(bottom_ * value), static_cast<int>(back_ * value),
            static_cast<int>(right_ * value), static_cast<int>(top_ * value), static_cast<int>(front_ * value)
        };
    }

    /// Add another bounding box.
    IntBoundingBox operator +(const IntBoundingBox& rhs) const
    {
        return {
            left_ + rhs.left_, bottom_ + rhs.bottom_, back_ + rhs.back_,
            right_ + rhs.right_, top_ + rhs.top_, front_ + rhs.front_
        };
    }

    /// Subtract another rect.
    IntBoundingBox operator -(const IntBoundingBox& rhs) const
    {
        return {
            left_ - rhs.left_, bottom_ - rhs.bottom_, back_ - rhs.back_,
            right_ - rhs.right_, top_ - rhs.top_, front_ - rhs.front_
        };
    }

    /// Return size.
    IntVector3 Size() const { return IntVector3{ Width(), Height(), Depth() }; }

    /// Return width.
    int Width() const { return right_ - left_; }

    /// Return height.
    int Height() const { return top_ - bottom_; }

    /// Return depth.
    int Depth() const { return front_ - back_; }

    /// Test whether a point is inside.
    Intersection IsInside(const IntVector3& point) const
    {
        if (point.x_ < left_   || point.x_ >= right_ ||
            point.y_ < bottom_ || point.y_ >= top_   ||
            point.z_ < back_   || point.z_ >= front_)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Clip with another rect.  Since IntBoundingBox does not have an undefined state
    /// like Rect, return (0, 0, 0, 0) if the result is empty.
    void Clip(const IntBoundingBox& rect);

    /// Merge a rect.  If this rect was empty, become the other rect.  If the
    /// other rect is empty, do nothing.
    void Merge(const IntBoundingBox& rect);

    /// Return integer data.
    const int* Data() const { return &left_; }

    /// Return as string.
    String ToString() const;

    /// Return left-top corner position.
    IntVector3 Min() const { return { left_, bottom_, back_ }; }

    /// Return right-bottom corner position.
    IntVector3 Max() const { return { right_, top_, front_ }; }

    /// Return left coordinate.
    int Left() const { return left_; }
    /// Return bottom coordinate.
    int Bottom() const { return bottom_; }
    /// Return bottom coordinate.
    int Back() const { return back_; }
    /// Return right coordinate.
    int Right() const { return right_; }
    /// Return top coordinate.
    int Top() const { return top_; }
    /// Return right coordinate.
    int Front() const { return front_; }


    /// Left coordinate.
    int left_;
    /// Bottom coordinate.
    int bottom_;
    /// Back coordinate.
    int back_;
    /// Right coordinate.
    int right_;
    /// Top coordinate.
    int top_;
    /// Front coordinate.
    int front_;

    /// Zero-sized rect.
    static const IntBoundingBox ZERO;
};

}
