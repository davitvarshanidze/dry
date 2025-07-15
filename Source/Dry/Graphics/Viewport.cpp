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

#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/View.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"

#include "../DebugNew.h"

namespace Dry
{

Viewport::Viewport(Context* context) :
    Object(context),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath((RenderPath*)nullptr);
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, RenderPath* renderPath) :
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath) :   // NOLINT(modernize-pass-by-value)
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(rect),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::~Viewport() = default;

void Viewport::SetScene(Scene* scene)
{
    scene_ = scene;
}

void Viewport::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void Viewport::SetCullCamera(Camera* camera)
{
    cullCamera_ = camera;
}

void Viewport::SetRect(const IntRect& rect)
{
    rect_ = rect;
}

void Viewport::SetDrawDebug(bool enable)
{
    drawDebug_ = enable;
}

void Viewport::SetRenderPath(RenderPath* renderPath)
{
    if (renderPath)
        renderPath_ = renderPath;
    else
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
            renderPath_ = renderer->GetDefaultRenderPath();
    }
}

bool Viewport::SetRenderPath(XMLFile* file)
{
    SharedPtr<RenderPath> newRenderPath(new RenderPath());
    if (newRenderPath->Load(file))
    {
        renderPath_ = newRenderPath;
        return true;
    }
    return false;
}

Scene* Viewport::GetScene() const
{
    return scene_;
}

Camera* Viewport::GetCamera() const
{
    return camera_;
}

Camera* Viewport::GetCullCamera() const
{
    return cullCamera_;
}

View* Viewport::GetView() const
{
    return view_;
}

RenderPath* Viewport::GetRenderPath() const
{
    return renderPath_;
}

IntVector2 Viewport::WorldToScreenPoint(const Vector3& worldPos) const
{
    if (!camera_)
        return IntVector2::ZERO;

    Graphics* graphics{ GetSubsystem<Graphics>() };
    Vector2 screenPos{ camera_->WorldToScreenPos(worldPos) };

    return graphics->ScreenPosToPoint(screenPos);
}

Ray Viewport::GetScreenRay(const IntVector2& screenPoint) const
{
    if (!camera_)
        return Ray();

    Vector2 screenPos{ GetSubsystem<Graphics>()->NormalizedScreenPos(screenPoint) };
    return camera_->GetScreenRay(screenPos);
}

Vector3 Viewport::ScreenToWorldPos(const IntVector2& screenPoint, float depth) const
{
    if (!camera_)
        return Vector3::ZERO;

    Vector2 screenPos{ GetSubsystem<Graphics>()->NormalizedScreenPos(screenPoint) };
    return camera_->ScreenToWorldPos(screenPos, depth);
}

void Viewport::AllocateView()
{
    view_ = new View(context_);
}

}
