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

#include "../Math/Vector3.h"

#include <cstdio>

#include "../DebugNew.h"

namespace Dry
{

const Vector3 Vector3::ZERO{};
const Vector3 Vector3::LEFT{ -1.0f, 0.0f, 0.0f };
const Vector3 Vector3::RIGHT{ 1.0f, 0.0f, 0.0f };
const Vector3 Vector3::UP{ 0.0f, 1.0f, 0.0f };
const Vector3 Vector3::DOWN{ 0.0f, -1.0f, 0.0f };
const Vector3 Vector3::FORWARD{ 0.0f, 0.0f, 1.0f };
const Vector3 Vector3::BACK{ 0.0f, 0.0f, -1.0f };
const Vector3 Vector3::ONE{ 1.0f, 1.0f, 1.0f };

const IntVector3 IntVector3::ZERO{};
const IntVector3 IntVector3::LEFT{ -1, 0, 0 };
const IntVector3 IntVector3::RIGHT{ 1, 0, 0 };
const IntVector3 IntVector3::UP{ 0, 1, 0 };
const IntVector3 IntVector3::DOWN{ 0, -1, 0 };
const IntVector3 IntVector3::FORWARD{ 0, 0, 1 };
const IntVector3 IntVector3::BACK{ 0, 0, -1 };
const IntVector3 IntVector3::ONE{ 1, 1, 1 };

Vector3::Vector3(const IntVector3& vector) noexcept:
    x_{ static_cast<float>(vector.x_) },
    y_{ static_cast<float>(vector.y_) },
    z_{ static_cast<float>(vector.z_) }
{
}

String Vector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g", x_, y_, z_);

    return String{ tempBuffer };
}

String IntVector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%d %d %d", x_, y_, z_);

    return String{ tempBuffer };
}

}
