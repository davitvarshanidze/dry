//
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

#include "../Math/Vector2.h"
#include "../Math/BoundingBox.h"
#include "../Container/Pair.h"

namespace Dry
{

enum PolynomialType{ PT_POLYNOMIAL, PT_HARMONIC_SIN, PT_HARMONIC_COS };

/// A polynomial expression.
class DRY_API Polynomial
{
public:
    /// Empty constructor.
    Polynomial(): Polynomial(0.f)
    {
    }

    /// Construct constant.
    Polynomial(float value,
               const Vector2& slope = Vector2::UP,
               PolynomialType type  = PT_POLYNOMIAL):
        Polynomial(PODVector<float>{ value }, slope, type)
    {
    }

    /// Constructor.
    Polynomial(const PODVector<float> coefficients,
               const Vector2& slope = Vector2::UP,
               PolynomialType type  = PT_POLYNOMIAL):
        coefficients_{ coefficients },
        slope_{ slope },
        polynomialType_{ type }
    {
    }

    /// Construct with expression type.
    Polynomial(PolynomialType type): Polynomial({ 0.f }, Vector2::UP, type)
    {
    }

    /// Solve the expression and return the result.
    float Solve(float t) const;
    /// Return the derived expression.
    Polynomial Derived() const;

    /// Set the coefficients.
    void SetCoefficients(const PODVector<float>& coefficients) { coefficients_ = coefficients; }
    /// Set the evaluation slope.
    void SetSlope(const Vector2& slope/*, bool correctInitial = true*/) { slope_ = slope; }
    /// Set polynomial type.
    void SetPolynomialType(PolynomialType type) { polynomialType_ = type; }

    /// Return polynomial coefficients const.
    const PODVector<float>& GetCoefficients() const { return coefficients_; }
    /// Return polynomial coefficients.
    PODVector<float>& GetCoefficients() { return coefficients_; }
    /// Return the evaluation slope const.
    const Vector2& GetSlope() const { return slope_; }
    /// Return the evaluation slope.
    Vector2& GetSlope() { return slope_; }
    /// Return polynomial type.
    PolynomialType GetPolynomialType() const { return polynomialType_; }

    /// Check polynomial for invariability.
    bool IsConstant() const;

protected:
    /// Coefficients of the polynomial.
    PODVector<float> coefficients_;
    /// Evaluation slope: x + y * t.
    Vector2 slope_;
    /// Polynomial/Harmonic evaluation.
    PolynomialType polynomialType_;
};

/// A statically typed polynomial.
template <PolynomialType type> class DRY_API StrictPolynomial: public Polynomial
{
public:
    void SetPolynomialType(PolynomialType) = delete;

    /// Constructor.
    StrictPolynomial(const PODVector<float> coefficients = { 0.f },
                     const Vector2& slope = Vector2::UP):
        Polynomial(coefficients, slope, type)
    {
    }
};

/// An N-dimensional polynomial expression.
template <unsigned n> class DRY_API NPolynomial: protected Vector<Polynomial>
{
public:
    /// Empty constructor.
    NPolynomial(): Vector<Polynomial>(n, {})
    {
    }

    /// Construct from single polynomial type.
    NPolynomial(PolynomialType type): Vector<Polynomial>(n, { type })
    {
    }

    /// Construct from a vector of polynomials.
    NPolynomial(const Vector<Polynomial>& polynomials, const Polynomial& fill = {}):
        Vector<Polynomial>(polynomials)
    {
        while (Size() > n)
            Pop();

        while (Size() < n)
            Push(fill);
    }

    /// Assign from another N-dimensional polynomial.
    NPolynomial<n>& operator =(const NPolynomial<n>& rhs)
    {
        if (&rhs != this)
        {
            for (unsigned i{ 0u }; i < n_; ++i)
                At(i) = rhs.At(i);
        }

        return *this;
    }

    /// Solve the expressions and return the result.
    PODVector<float> Solve(float t) const
    {
        float res[n_];

        for (unsigned i{ 0u }; i < n_; ++i)
            res[i] = At(i).Solve(t);

        return PODVector<float>{ res, n_ };
    }

    /// Return derivative.
    NPolynomial<n> Derived() const
    {
        NPolynomial<n> derivative{ *this };

        for (unsigned i{ 0u }; i < n_; ++i)
            derivative.At(i) = derivative.At(i).Derived();

        return derivative;
    }

    /// Set the Nth polynomial.
    void SetPolynomial(unsigned index, const Polynomial& polynomial)
    {
        At(index) = polynomial;
    }
    /// Set the coefficients.
    void SetCoefficients(unsigned index, const PODVector<float>& coefficients)
    {
        if (index < n_)
            At(index).SetCoefficients(coefficients);
    }

    /// Set all evaluation slopes.
    void SetSlope(const Vector2& slope/*, bool correctInitial = true*/)
    {
        for (int i{ 0 }; i < n_; ++i)
            SetSlope(i, slope);
    }

    /// Set the evaluation slope.
    void SetSlope(unsigned index, const Vector2& slope/*, bool correctInitial = true*/)
    {
        if (index < n_)
            At(index).SetSlope(slope);
    }
    /// Set multiple evaluation slopes simultaneously.
    void SetSlope(const PODVector<Vector2>& slopes/*, bool correctInitial = true*/)
    {
        for (int i{ 0 }; i < n_ && i < slopes.Size(); ++i)
            At(i).SetSlope(slopes.At(i));
    }
    /// Set polynomial type.
    void SetPolynomialType(unsigned index, PolynomialType type)
    {
        if (index < n_)
            At(index).SetPolynomialType(type);
    }

    /// Return the const Nth polynomial.
    const Polynomial& GetPolynomial(unsigned index) const { return At(index); }
    /// Return the Nth polynomial.
    Polynomial& GetPolynomial(unsigned index) { return At(index); }
    /// Return const polynomial coefficients of index.
    const PODVector<float>&  GetCoefficients(unsigned index) const { return At(index).GetCoefficients(); }
    /// Return polynomial coefficients of index.
    PODVector<float>& GetCoefficients(unsigned index) { return At(index).GetCoefficients(); }
    /// Return the evaluation slope of index.
    Vector2 GetSlope(unsigned index) const { return At(index).GetSlope(); }
    /// Return polynomial type of index.
    PolynomialType GetPolynomialType(unsigned index) const { return At(index).GetPolynomialType(); }

    /// Return the number of dimensions of the N-polynomial.
    unsigned GetDimensions() const { return n_; }

    /// Check for invariability.
    bool IsConstant() const
    {
        for (unsigned i{ 0u }; i < n_; ++i)
            if (!IsConstant(i))
                return false;

        return true;
    }

    /// Check for invariability of index.
    bool IsConstant(unsigned i) const
    {
        if (!At(i).IsConstant())
            return false;

        return true;
    }

protected:
    /// Construct from a list of starting values.
    NPolynomial(const float* data):
        NPolynomial()
    {
        for (unsigned i{ 0u }; i < n_; ++i)
            SetPolynomial(i, { data[i] });
    }

    /// Number of dimensions.
    const unsigned n_{ n };
};

/// A typed polynomial expression.
template <class T> class DRY_API TypedPolynomial: public NPolynomial<sizeof(T) / sizeof(float)>
{
public:
    static constexpr size_t TypeSize() { return sizeof(T) / sizeof(float); }
    using ValueType = T;
    using NPolynomial<TypeSize()>::NPolynomial;

    /// Construct a constant.
    TypedPolynomial(const T& value):
        NPolynomial<TypeSize()>(value.Data())
    {
    }

    /// Construct from a range of coefficients.
    TypedPolynomial(PODVector<T> coefficients):
        NPolynomial<TypeSize()>()
    {
        for (unsigned i{ 0 }; i < coefficients.Size(); ++i)
            SetCoefficient(i, coefficients.At(i));
    }

    /// Set a single coefficient for each polynomial.
    void SetCoefficient(unsigned index, const T& value)
    {
        const float* data{ value.Data() };

        for (int i{ 0 }; i < TypeSize(); ++i)
        {
            const float a{ data[i] };
            PODVector<float>& coefficients{ TypedPolynomial::GetCoefficients(i) };

            while (coefficients.Size() < index + 1u)
                coefficients.Push(0.f);

            coefficients.At(index) = a;

            while (coefficients.Size() && coefficients.Back() == 0.f)
                coefficients.Pop();

            TypedPolynomial::SetCoefficients(i, coefficients);
        }
    }

    /// Return the combined coefficients at index.
    T GetCoefficient(unsigned index) const
    {
        return T{ NPolynomial<TypeSize()>::GetCoefficient(index).Buffer() };
    }

//    PODVector<float> Solve(float t) = delete;
    /// Solve the expressions and return the result.
    T Solve(float t) const
    {
        return T{ NPolynomial<TypeSize()>::Solve(t).Buffer() };
    }

//    NPolynomial<TypeSize()> Derived() = delete;
    /// Return the derived typed polynomial.
    TypedPolynomial<T> Derived() const
    {
        return TypedPolynomial<T>{ NPolynomial<TypeSize()>::Derived() };
    }
};

/// A pair of polynomial expressions bound to a type.
template <class T> class DRY_API TypedBipolynomial: protected Pair<TypedPolynomial<T>, TypedPolynomial<T> >
{
public:
    static constexpr size_t TypeSize() { return TypedPolynomial<T>::TypeSize(); }
    using ValueType = T;

    /// Empty constructor.
    explicit TypedBipolynomial(bool firstDominant = true):
        Pair<TypedPolynomial<T>, TypedPolynomial<T> >({}, {}),
        firstDominant_{ firstDominant }
    {
    }

    /// Construct a constant.
    explicit TypedBipolynomial(const T& value, bool firstDominant = true):
        TypedBipolynomial(value, value, firstDominant)
    {
    }

    /// Construct from two constant.
    explicit TypedBipolynomial(const T& first, const T& second, bool firstDominant = true):
        TypedBipolynomial(TypedPolynomial<T>{ first },
                          TypedPolynomial<T>{ second },
                          firstDominant)
    {
    }

    /// Construct from two typed polynomials.
    TypedBipolynomial(const TypedPolynomial<T>& first, const TypedPolynomial<T>& second, bool firstDominant = true):
        Pair<TypedPolynomial<T>, TypedPolynomial<T> >(first, second),
        firstDominant_{ firstDominant }
    {
        Harmonize();
    }

    /// Construct from a single typed polynomials.
    TypedBipolynomial(const TypedPolynomial<T>& typedPolynomial, bool firstDominant = true):
        TypedBipolynomial(typedPolynomial, typedPolynomial, firstDominant)
    {
    }

    /// Assign from another typed bipolynomial.
    TypedBipolynomial<T>& operator =(const TypedBipolynomial<T>& rhs)
    {
        if (&rhs != this)
        {
            TypedBipolynomial::first_  = rhs.first_;
            TypedBipolynomial::second_ = rhs.second_;
            firstDominant_ = rhs.firstDominant_;
        }

        return *this;
    }

    /// Solve the expression and return the result.
    Pair<T, T> Solve(float t)
    {
        return { TypedBipolynomial::first_ .Solve(t),
                 TypedBipolynomial::second_.Solve(t) };
    }

    /// Return derived typed bipolynomial.
    TypedBipolynomial Derived()
    {
        return { TypedBipolynomial::first_ .Derived(),
                 TypedBipolynomial::second_.Derived() };
    }

    /// Randomly recombine and return the result.
    TypedPolynomial<T> ExtractRandom(const IntVector2& coefficientRange = IntVector2::ONE * M_MAX_INT) const
    {
        TypedPolynomial<T> res{};

        const TypedPolynomial<T>& min{ TypedBipolynomial::first_ };
        const TypedPolynomial<T>& max{ TypedBipolynomial::second_ };

        for (unsigned i{ 0u }; i < TypeSize(); ++i)
        {
            const PODVector<float> minCoefficients{ min.GetCoefficients(i) };
            const PODVector<float> maxCoefficients{ max.GetCoefficients(i) };
            const unsigned numCoefficients{ Min(Max(minCoefficients.Size(), maxCoefficients.Size()),
                                                Max(0, Random(coefficientRange.x_, coefficientRange.y_ + 1))) };

            PODVector<float> coefficients{};
            for (unsigned c{ 0u }; c < numCoefficients; ++c)
            {
                const float minC{ (c < minCoefficients.Size() ? minCoefficients.At(c) : 0.f) };
                const float maxC{ (c < maxCoefficients.Size() ? maxCoefficients.At(c) : 0.f) };
                coefficients.Push(Random(minC, maxC));
            }

            while (coefficients.Size() && coefficients.Back() == 0.f)
                coefficients.Pop();

            const Vector2 slope{ min.GetSlope(i).Lerp(max.GetSlope(i), Random()) };

            res.SetPolynomial(i, { coefficients, slope, GetPolynomialType(i) });
        }

        return res;
    }

    /// Recombine lerped and return the result.
    TypedPolynomial<T> ExtractLerped(float t) const
    {
        TypedPolynomial<T> res{};

        const TypedPolynomial<T>& lhs{ TypedBipolynomial::first_ };
        const TypedPolynomial<T>& rhs{ TypedBipolynomial::second_ };

        for (unsigned i{ 0 }; i < TypeSize(); ++i)
        {
            const PODVector<float> lhsCoefficients{ lhs.GetCoefficients(i) };
            const PODVector<float> rhsCoefficients{ rhs.GetCoefficients(i) };
            const unsigned numCoefficients{ Max(lhsCoefficients.Size(),
                                                rhsCoefficients.Size()) };

            PODVector<float> coefficients{};
            for (unsigned c{ 0u }; c < numCoefficients; ++c)
            {
                const float lhsC{ (c < lhsCoefficients.Size() ? lhsCoefficients.At(c) : 0.f) };
                const float rhsC{ (c < rhsCoefficients.Size() ? rhsCoefficients.At(c) : 0.f) };
                coefficients.Push(Lerp(lhsC, rhsC, t));
            }

            while (coefficients.Size() && coefficients.Back() == 0.f)
                coefficients.Pop();

            const Vector2 slope{ lhs.GetSlope(i).Lerp(rhs.GetSlope(i), t) };

            res.SetPolynomial(i, { coefficients, slope, GetPolynomialType(i) });
        }

        return res;
    }

    /// Set whether the first or second typed polynmial is harmonically dominant.
    void SetFirstDominant(bool firstDominant) { firstDominant_ = firstDominant; }

    /// Set the first typed polynmial.
    void SetFirst(const TypedPolynomial<T>& first)
    {
        TypedBipolynomial::first_ = first;
        Harmonize();
    }

    /// Set the second typed polynmial.
    void SetSecond(const TypedPolynomial<T>& first)
    {
        TypedBipolynomial::first_ = first;
        Harmonize();
    }

    /// Set the polynomial type on the same index for both typed polynomials.
    void SetPolynomialType(unsigned index, PolynomialType type)
    {
        TypedBipolynomial::first_.SetPolynomialType(index, type);
        TypedBipolynomial::second_.SetPolynomialType(index, type);
    }

private:
    /// Ensure harmonic equality.
    void Harmonize()
    {
        for (unsigned i{ 0 }; i < TypeSize(); ++i)
            Harmonize(i);
    }

    /// Ensure harmonic equality at index.
    void Harmonize(unsigned index)
    {
        const PolynomialType type{ GetPolynomialType(index) };

        if (firstDominant_)
            TypedBipolynomial::second_.SetPolynomialType(index, type);
        else
            TypedBipolynomial::first_.SetPolynomialType(index, type);
    }

    /// Get type of polynomial/harmonic expression at index.
    PolynomialType GetPolynomialType(unsigned index) const
    {
        if (firstDominant_)
            return TypedBipolynomial::first_ .GetPolynomialType(index);
        else
            return TypedBipolynomial::second_.GetPolynomialType(index);
    }

    bool firstDominant_;
};

/// A typed field with typed values controlled by harmonics and polynomials.
template <class T> class DRY_API HarmonicField
{

public:
    enum Extrapolation{ EXTRA_DEFAULT = 0, EXTRA_CLAMP, EXTRA_INFINITE };

    /// Empty constructor.
    HarmonicField():
        bounds_{},
        xPolynomial_{},
        yPolynomial_{},
        zPolynomial_{},
        defaultValue_{ T{} }
    {
    }

    /// Construct from three typed polynomials and, optionally, default value.
    HarmonicField(const TypedPolynomial<T>& x,
                  const TypedPolynomial<T>& y,
                  const TypedPolynomial<T>& z,
                  const T& abyss = T{}):
        bounds_{},
        xPolynomial_{ x },
        yPolynomial_{ y },
        zPolynomial_{ z },
        defaultValue_{ T{} }
    {
    }

    /// Calculate and return the result for a given position.
    T Solve(const Vector3& position)
    {
        const Vector3 origin{ bounds_.min_ };

        const T resX{ xPolynomial_.Solve(position.x_ - origin.x_) };
        const T resY{ yPolynomial_.Solve(position.y_ - origin.y_) };
        const T resZ{ zPolynomial_.Solve(position.z_ - origin.z_) };
        const PODVector<T> results{ resX, resY, resZ };
        return Average(results.Begin(), results.End());
    }

private:
    /// Bounding box defining the area covered by the field.
    BoundingBox bounds_;

    /// Typed polynomial mapped onto the X-axis.
    TypedPolynomial<T> xPolynomial_;
    /// Typed polynomial mapped onto the Y-axis.
    TypedPolynomial<T> yPolynomial_;
    /// Typed polynomial mapped onto the Z-axis.
    TypedPolynomial<T> zPolynomial_;
    /// Out of bounds return value for default extrapolation mode.
    T defaultValue_;
};

}
