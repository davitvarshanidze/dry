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

#include "../Container/Str.h"
#include "../Math/MathDefs.h"

namespace Dry
{
class IntVector2;

/// Two-dimensional vector.
class DRY_API Vector2
{
public:
    /// Construct a zero vector.
    Vector2() noexcept :
        x_{ 0.0f },
        y_{ 0.0f }
    {
    }

    /// Copy-construct from another vector.
    Vector2(const Vector2& vector) noexcept = default;

    /// Construct from an IntVector2.
    explicit Vector2(const IntVector2& vector) noexcept;

    /// Construct from coordinates.
    Vector2(float x, float y) noexcept:
        x_{ x },
        y_{ y }
    {
    }

    /// Construct from a float array.
    explicit Vector2(const float* const data) noexcept :
        x_{ data[0] },
        y_{ data[1] }
    {
    }

    /// Assign from another vector.
    Vector2& operator =(const Vector2& rhs) noexcept = default;

    /// Test for equality with another vector without epsilon.
    bool operator ==(const Vector2& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_; }
    /// Test for inequality with another vector without epsilon.
    bool operator !=(const Vector2& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_; }

    /// Add a vector.
    Vector2 operator +(const Vector2& rhs) const { return Vector2{ x_ + rhs.x_, y_ + rhs.y_ }; }

    /// Return negation.
    Vector2 operator -() const { return Vector2{ -x_, -y_ }; }
    /// Subtract a vector.
    Vector2 operator -(const Vector2& rhs) const { return Vector2{ x_ - rhs.x_, y_ - rhs.y_ }; }

    /// Multiply with a scalar.
    Vector2 operator *(float rhs) const { return Vector2{ x_ * rhs, y_ * rhs }; }
    /// Multiply with a vector.
    Vector2 operator *(const Vector2& rhs) const { return Vector2{ x_ * rhs.x_, y_ * rhs.y_ }; }

    /// Divide by a scalar.
    Vector2 operator /(float rhs) const { return Vector2{ x_ / rhs, y_ / rhs }; }
    /// Divide by a vector.
    Vector2 operator /(const Vector2& rhs) const { return Vector2{ x_ / rhs.x_, y_ / rhs.y_ }; }

    /// Add-assign a vector.
    Vector2& operator +=(const Vector2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;

        return *this;
    }

    /// Subtract-assign a vector.
    Vector2& operator -=(const Vector2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;

        return *this;
    }

    /// Multiply-assign a scalar.
    Vector2& operator *=(float rhs)
    {
        x_ *= rhs;
        y_ *= rhs;

        return *this;
    }

    /// Multiply-assign a vector.
    Vector2& operator *=(const Vector2& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;

        return *this;
    }

    /// Divide-assign a scalar.
    Vector2& operator /=(float rhs)
    {
        const float invRhs{ 1.0f / rhs };
        x_ *= invRhs;
        y_ *= invRhs;

        return *this;
    }

    /// Divide-assign a vector.
    Vector2& operator /=(const Vector2& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;

        return *this;
    }

    /// Normalize to unit length.
    void Normalize()
    {
        const float lenSquared{ LengthSquared() };

        if (!Dry::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            const float invLen{ 1.0f / sqrtf(lenSquared) };
            x_ *= invLen;
            y_ *= invLen;
        }
    }

    /// Return length.
    float Length() const { return sqrtf(x_ * x_ + y_ * y_); }

    /// Return squared length.
    float LengthSquared() const { return x_ * x_ + y_ * y_; }

    /// Calculate dot product.
    float DotProduct(const Vector2& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_; }

    /// Calculate absolute dot product.
    float AbsDotProduct(const Vector2& rhs) const { return Dry::Abs(x_ * rhs.x_) + Dry::Abs(y_ * rhs.y_); }

    /// Project vector onto axis.
    float ProjectOntoAxis(const Vector2& axis) const { return DotProduct(axis.Normalized()); }

    /// Returns the angle between this vector and another vector in degrees.
    float Angle(const Vector2& rhs) const { return Dry::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Return rotated.
    Vector2 Rotated(float angle) const
    {
        const float cs{ Cos(angle) };
        const float sn{ Sin(angle) };

        return Vector2(x_ * cs - y_ * sn, x_ * sn + y_ * cs);
    }

    /// Return absolute vector.
    Vector2 Abs() const { return Vector2{ Dry::Abs(x_), Dry::Abs(y_) }; }

    /// Linear interpolation with another vector.
    Vector2 Lerp(const Vector2& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector2& rhs) const { return Dry::Equals(x_, rhs.x_) && Dry::Equals(y_, rhs.y_); }

    /// Return whether any component is NaN.
    bool IsNaN() const { return Dry::IsNaN(x_) || Dry::IsNaN(y_); }

    /// Return whether any component is Inf.
    bool IsInf() const { return Dry::IsInf(x_) || Dry::IsInf(y_); }

    /// Return normalized to unit length.
    Vector2 Normalized() const
    {
        const float lenSquared{ LengthSquared() };

        if (!Dry::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            const float invLen{ 1.0f / sqrtf(lenSquared) };

            return *this * invLen;
        }
        else
        {
            return *this;
        }
    }

    /// Return normalized to unit length or zero if length is too small.
    Vector2 NormalizedOrDefault(const Vector2& defaultValue = Vector2::ZERO, float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared{ LengthSquared() };

        if (lenSquared < eps * eps)
            return defaultValue;

        return *this / sqrtf(lenSquared);
    }

    /// Return normalized vector with length in given range.
    Vector2 ReNormalized(float minLength, float maxLength, const Vector2& defaultValue = Vector2::ZERO, float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared{ LengthSquared() };

        if (lenSquared < eps * eps)
            return defaultValue;

        const float len{ sqrtf(lenSquared) };
        const float newLen{ Clamp(len, minLength, maxLength) };
        return *this * (newLen / len);
    }

    /// Return float data.
    const float* Data() const { return &x_; }

    /// Return as string.
    String ToString() const;

    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;

    /// Zero vector.
    static const Vector2 ZERO;
    /// (-1,0) vector.
    static const Vector2 LEFT;
    /// (1,0) vector.
    static const Vector2 RIGHT;
    /// (0,1) vector.
    static const Vector2 UP;
    /// (0,-1) vector.
    static const Vector2 DOWN;
    /// (1,1) vector.
    static const Vector2 ONE;
};

/// Two-dimensional vector with integer values.
class DRY_API IntVector2
{
public:
    /// Construct a zero vector.
    IntVector2() noexcept :
        x_{ 0 },
        y_{ 0 }
    {
    }

    /// Construct from coordinates.
    IntVector2(int x, int y) noexcept :
        x_{ x },
        y_{ y }
    {
    }

    /// Construct from an int array.
    explicit IntVector2(const int* const data) noexcept :
        x_{ data[0] },
        y_{ data[1] }
    {
    }

    /// Construct from an float array.
    explicit IntVector2(const float* const data) :
        x_{ static_cast<int>(data[0]) },
        y_{ static_cast<int>(data[1]) }
    {
    }

    /// Copy-construct from another vector.
    IntVector2(const IntVector2& rhs) noexcept = default;

    /// Assign from another vector.
    IntVector2& operator =(const IntVector2& rhs) noexcept = default;

    /// Test for equality with another vector.
    bool operator ==(const IntVector2& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_; }
    /// Test for inequality with another vector.
    bool operator !=(const IntVector2& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_; }

    /// Add a vector.
    IntVector2 operator +(const IntVector2& rhs) const { return IntVector2{ x_ + rhs.x_, y_ + rhs.y_ }; }
    /// Add a vector.
    Vector2 operator +(const Vector2& rhs) const { return Vector2{ x_ + rhs.x_, y_ + rhs.y_ }; }

    /// Return negation.
    IntVector2 operator -() const { return IntVector2{ -x_, -y_ }; }
    /// Subtract a vector.
    IntVector2 operator -(const IntVector2& rhs) const { return IntVector2{ x_ - rhs.x_, y_ - rhs.y_ }; }
    /// Subtract a vector.
    Vector2 operator -(const Vector2& rhs) const { return Vector2{ x_ - rhs.x_, y_ - rhs.y_ }; }

    /// Multiply with an int scalar.
    IntVector2 operator *(int rhs) const { return IntVector2{ x_ * rhs, y_ * rhs }; }
    /// Multiply with a float scalar.
    Vector2 operator *(float rhs) const { return Vector2{ x_ * rhs, y_ * rhs }; }
    /// Multiply with another int vector.
    IntVector2 operator *(const IntVector2& rhs) const { return IntVector2{ x_ * rhs.x_, y_ * rhs.y_ }; }
    /// Multiply with a float vector.
    Vector2 operator *(const Vector2& rhs) const { return Vector2{ x_ * rhs.x_, y_ * rhs.y_ }; }

    /// Divide by an int scalar.
    IntVector2 operator /(int rhs) const { return IntVector2{ x_ / rhs, y_ / rhs }; }
    /// Divide by a float scalar.
    Vector2 operator /(float rhs) const { return Vector2{ x_ / rhs, y_ / rhs }; }
    /// Divide by another int vector.
    IntVector2 operator /(const IntVector2& rhs) const { return IntVector2{ x_ / rhs.x_, y_ / rhs.y_ }; }
    /// Divide by a float vector.
    Vector2 operator /(const Vector2& rhs) const { return Vector2{ x_ / rhs.x_, y_ / rhs.y_ }; }

    /// Add-assign a vector.
    IntVector2& operator +=(const IntVector2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    /// Subtract-assign a vector.
    IntVector2& operator -=(const IntVector2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    /// Multiply-assign a scalar.
    IntVector2& operator *=(int rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    IntVector2& operator *=(const IntVector2& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        return *this;
    }

    /// Divide-assign a scalar.
    IntVector2& operator /=(int rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        return *this;
    }

    /// Divide-assign a vector.
    IntVector2& operator /=(const IntVector2& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        return *this;
    }

    /// Return transposed.
    IntVector2 Transposed() const { return { y_, x_ }; }

    /// Return rotated in steps of 90 degrees.
    IntVector2 Rotated(int steps) const
    {
        while (steps < -3)
            steps += 4;
        while (steps > 3)
            steps -= 4;

        IntVector2 res{ x_, y_ };
        while (steps++ < 0)
            res = { -res.y_, res.x_ };
        while (steps-- > 0)
            res = { res.y_, -res.x_ };

        return res;
    }

    /// Return integer data.
    const int* Data() const { return &x_; }

    /// Return as string.
    String ToString() const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return x_ * 31u + y_; }

    /// Return length.
    float Length() const { return sqrtf(static_cast<float>(x_ * x_ + y_ * y_)); }

    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;

    /// Zero vector.
    static const IntVector2 ZERO;
    /// (-1,0) vector.
    static const IntVector2 LEFT;
    /// (1,0) vector.
    static const IntVector2 RIGHT;
    /// (0,1) vector.
    static const IntVector2 UP;
    /// (0,-1) vector.
    static const IntVector2 DOWN;
    /// (1,1) vector.
    static const IntVector2 ONE;
};

/// Add a Vector2 to an IntVector2.
inline Vector2 operator +(const Vector2& lhs, const IntVector2& rhs) { return rhs + lhs; }
/// Substract a Vector2 from an IntVector2.
inline Vector2 operator -(const Vector2& lhs, const IntVector2& rhs) { return lhs - Vector2{ rhs }; }
/// Multiply Vector2 with a scalar
inline Vector2 operator *(float lhs, const Vector2& rhs) { return rhs * lhs; }
/// Multiply IntVector2 with a scalar.
inline IntVector2 operator *(int lhs, const IntVector2& rhs) { return rhs * lhs; }
/// Multiply float scalar  with an IntVector2.
inline Vector2 operator *(float lhs, const IntVector2& rhs) { return rhs * lhs; }
/// Multiply Vector2 with an IntVector2.
inline Vector2 operator *(const Vector2& lhs, const IntVector2& rhs) { return rhs * lhs; }
/// Divide Vector2 by an IntVector2.
inline Vector2 operator /(const Vector2& lhs, const IntVector2& rhs) { return lhs / Vector2{ rhs }; }

/// Per-component linear interpolation between two 2-vectors.
inline Vector2 VectorLerp(const Vector2& lhs, const Vector2& rhs, const Vector2& t) { return lhs + (rhs - lhs) * t; }
/// Per-component min of two 2-vectors.
inline Vector2 VectorMin(const Vector2& lhs, const Vector2& rhs) { return Vector2{ Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_) }; }
/// Per-component max of two 2-vectors.
inline Vector2 VectorMax(const Vector2& lhs, const Vector2& rhs) { return Vector2{ Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_) }; }
/// Per-component floor of 2-vector.
inline Vector2 VectorFloor(const Vector2& vec) { return Vector2{ Floor(vec.x_), Floor(vec.y_) }; }
/// Per-component round of 2-vector.
inline Vector2 VectorRound(const Vector2& vec) { return Vector2{ Round(vec.x_), Round(vec.y_) }; }
/// Per-component ceil of 2-vector.
inline Vector2 VectorCeil(const Vector2& vec) { return Vector2{ Ceil(vec.x_), Ceil(vec.y_) }; }
/// Per-component absolute value of 2-vector.
inline Vector2 VectorAbs(const Vector2& vec) { return Vector2{ Abs(vec.x_), Abs(vec.y_) }; }
/// Per-component sign of 2-vector.
inline Vector2 VectorSign(const Vector2& vec) { return Vector2{ Sign(vec.x_), Sign(vec.y_) }; }

/// Per-component clamp of a 2-vector.
inline Vector2 VectorClamp(const Vector2& vec, const Vector2& min, const Vector2& max) { return VectorMin(VectorMax(vec, min), max); }
/// Per-component remainder of two 2-vectors.
inline Vector2 VectorMod(const Vector2& lhs, const Vector2& rhs) { return Vector2{ Mod(lhs.x_, rhs.x_), Mod(lhs.y_, rhs.y_) }; }
/// Per-component remainder of a 2-vector and float value.
inline Vector2 VectorMod(const Vector2& lhs, float rhs) { return Vector2{ Mod(lhs.x_, rhs), Mod(lhs.y_, rhs) }; }

/// Per-component floor of 2-vector. Returns IntVector2.
inline IntVector2 VectorFloorToInt(const Vector2& vec) { return IntVector2{ FloorToInt(vec.x_), FloorToInt(vec.y_) }; }
/// Per-component round of 2-vector. Returns IntVector2.
inline IntVector2 VectorRoundToInt(const Vector2& vec) { return IntVector2{ RoundToInt(vec.x_), RoundToInt(vec.y_) }; }
/// Per-component ceil of 2-vector. Returns IntVector2.
inline IntVector2 VectorCeilToInt(const Vector2& vec) { return IntVector2{ CeilToInt(vec.x_), CeilToInt(vec.y_) }; }
/// Per-component min of two 2-vectors.
inline IntVector2 VectorMin(const IntVector2& lhs, const IntVector2& rhs) { return IntVector2{ Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_) }; }
/// Per-component max of two 2-vectors.
inline IntVector2 VectorMax(const IntVector2& lhs, const IntVector2& rhs) { return IntVector2{ Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_) }; }
/// Per-component absolute value of integer 2-vector.
inline IntVector2 VectorAbs(const IntVector2& vec) { return IntVector2{ Abs(vec.x_), Abs(vec.y_) }; }
/// Per-component sign of integer 2-vector.
inline IntVector2 VectorSign(const IntVector2& vec) { return IntVector2{ Sign(vec.x_), Sign(vec.y_) }; }

/// Per-component clamp of a 2-vector.
inline IntVector2 VectorClamp(const IntVector2& vec, const IntVector2& min, const IntVector2& max) { return VectorMin(VectorMax(vec, min), max); }
/// Per-component remainder of two 2-vectors.
inline IntVector2 VectorMod(const IntVector2& lhs, const IntVector2& rhs) { return IntVector2{ Mod(lhs.x_, rhs.x_), Mod(lhs.y_, rhs.y_) }; }
/// Per-component remainder of a 2-vector and float value.
inline IntVector2 VectorMod(const IntVector2& lhs, int rhs) { return IntVector2{ Mod(lhs.x_, rhs), Mod(lhs.y_, rhs) }; }

/// Return a random value from [0, 1) from 2-vector seed.
/// http://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
inline float StableRandom(const Vector2& seed) { return Fract(Sin(seed.DotProduct(Vector2{ 12.9898f, 78.233f }) * M_RADTODEG) * 43758.5453f); }

/// Return a random value from [0, 1) from scalar seed.
inline float StableRandom(float seed) { return StableRandom(Vector2{ seed, seed }); }

}
