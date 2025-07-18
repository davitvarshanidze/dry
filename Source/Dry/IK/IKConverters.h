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

#include "../Math/Quaternion.h"
#include "../Math/Vector3.h"

#include <ik/quat.h>
#include <ik/vec3.h>

namespace Dry {

/// Converts from an Dry Vector3 to an IK vec3_t
vec3_t Vec3Dry2IK(const Vector3& urho);
/// Converts from an IK vec3_t to an Dry Vector3
Vector3 Vec3IK2Dry(const vec3_t* ik);
/// Converts from an Dry quaternion to an IK quat_t
quat_t QuatDry2IK(const Quaternion& urho);
/// Converts from an IK quat_t to an Dry quaternion
Quaternion QuatIK2Dry(const quat_t* ik);

}
