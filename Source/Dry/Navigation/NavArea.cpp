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
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Navigation/NavArea.h"
#include "../Scene/Node.h"

namespace Dry
{

static const unsigned MAX_NAV_AREA_ID = 255;
static const Vector3 DEFAULT_BOUNDING_BOX_MIN(-10.0f, -10.0f, -10.0f);
static const Vector3 DEFAULT_BOUNDING_BOX_MAX(10.0f, 10.0f, 10.0f);
static const unsigned DEFAULT_AREA_ID = 0;

extern const char* DRY_NAVIGATION_CATEGORY;

NavArea::NavArea(Context* context) :
    Component(context),
    areaID_(DEFAULT_AREA_ID),
    boundingBox_(DEFAULT_BOUNDING_BOX_MIN, DEFAULT_BOUNDING_BOX_MAX)
{
}

NavArea::~NavArea() = default;

void NavArea::RegisterObject(Context* context)
{
    context->RegisterFactory<NavArea>(DRY_NAVIGATION_CATEGORY);

    DRY_COPY_BASE_ATTRIBUTES(Component);
    DRY_ATTRIBUTE("Bounding Box Min", Vector3, boundingBox_.min_, DEFAULT_BOUNDING_BOX_MIN, AM_DEFAULT);
    DRY_ATTRIBUTE("Bounding Box Max", Vector3, boundingBox_.max_, DEFAULT_BOUNDING_BOX_MAX, AM_DEFAULT);
    DRY_ACCESSOR_ATTRIBUTE("Area ID", GetAreaID, SetAreaID, unsigned, DEFAULT_AREA_ID, AM_DEFAULT);
}

void NavArea::SetAreaID(unsigned newID)
{
    if (newID > MAX_NAV_AREA_ID)
        DRY_LOGERRORF("NavArea Area ID %u exceeds maximum value of %u", newID, MAX_NAV_AREA_ID);
    areaID_ = (unsigned char)newID;
    MarkNetworkUpdate();
}

BoundingBox NavArea::GetWorldBoundingBox() const
{
    Matrix3x4 mat;
    mat.SetTranslation(node_->GetWorldPosition());
    return boundingBox_.Transformed(mat);
}

void NavArea::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        Matrix3x4 mat;
        mat.SetTranslation(node_->GetWorldPosition());
        debug->AddBoundingBox(boundingBox_, mat, Color::GREEN, depthTest);
        debug->AddBoundingBox(boundingBox_, mat, Color(0.0f, 1.0f, 0.0f, 0.15f), true, true);
    }
}

}
