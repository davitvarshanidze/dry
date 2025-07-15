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

#include "Polynomial.h"

namespace Dry
{

float Polynomial::Solve(float t) const
{
    float res{};

    for (unsigned f{ 0u }; f < coefficients_.Size(); ++f)
    {
        float a{ coefficients_.At(f) };

        if (a == 0.f)
            continue;

        const float x{ slope_.x_ + slope_.y_ * t };

        if (polynomialType_ == PT_POLYNOMIAL)
        {
            for (unsigned i{ 0u }; i < f; ++i)
                a *= x;

            res += a;
        }
        else
        {
            if (f == 0u)
            {
                res += a;
            }
            else
            {
                if (polynomialType_ == PT_HARMONIC_SIN)
                    res += a * Sin(f * x * M_RADTODEG * M_TAU);
                else
                    res += a * Cos(f * x * M_RADTODEG * M_TAU);
            }
        }
    }

    return res;
}

Polynomial Polynomial::Derived() const
{
    PODVector<float> coefficients{};

    if (polynomialType_ != PT_POLYNOMIAL)
        coefficients.Push(0.f);

    for (unsigned i{ 1u }; i < coefficients_.Size(); ++i)
    {
        if (polynomialType_ == PT_POLYNOMIAL)
            coefficients.Push(coefficients_.At(i) * slope_.y_);
        else
            coefficients.Push(coefficients_.At(i) * slope_.y_ * i);
    }

    PolynomialType derivativeType{ (polynomialType_ == PT_POLYNOMIAL   ? PT_POLYNOMIAL :
                                    polynomialType_ == PT_HARMONIC_SIN ? PT_HARMONIC_COS
                                                                       : PT_HARMONIC_SIN) };
    return { coefficients, slope_, derivativeType };
}

bool Polynomial::IsConstant() const
{
    if (slope_.y_ == 0.f)
        return true;

    for (unsigned i{ 1u }; i < coefficients_.Size(); ++i)
    {
        if (coefficients_.At(i) != 0.f)
            return false;
    }

    return true;
}

}
