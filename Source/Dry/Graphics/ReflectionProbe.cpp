//
// Copyright (c) 2016-2017 Mesh Consultants Inc.
// Copyright (c) 2024-2024 LucKey Productions.
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
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/ReflectionProbe.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/Zone.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"

#include "../DebugNew.h"

namespace Dry
{

extern const char* DRY_SCENE_CATEGORY;
extern const char* filterModeNames[];

ReflectionProbe::ReflectionProbe(Context* context): Component(context),
    gimbal_{ nullptr },
    cameras_{},
    nearClip_{ DEFAULT_NEARCLIP },
    farClip_{ DEFAULT_FARCLIP },
    viewMask_{ M_MAX_UNSIGNED },
    viewports_{},
    reflectionMap_{ nullptr },
    resolution_{ 64u },
    filterMode_{ FILTER_BILINEAR }
{
}

ReflectionProbe::~ReflectionProbe()
{
    if (gimbal_)
        gimbal_->Remove();

    for (Viewport* vp: viewports_)
        vp->ReleaseRef();
}


void ReflectionProbe::RegisterObject(Context* context)
{
    context->RegisterFactory<ReflectionProbe>(DRY_SCENE_CATEGORY);

    DRY_ATTRIBUTE("Near Clip", float, nearClip_, DEFAULT_NEARCLIP, AM_DEFAULT);
    DRY_ATTRIBUTE("Far Clip", float, farClip_, DEFAULT_FARCLIP, AM_DEFAULT);
    DRY_ATTRIBUTE("View Mask", unsigned, viewMask_, M_MAX_UNSIGNED, AM_DEFAULT);
    DRY_ATTRIBUTE("Resolution", unsigned, resolution_, 64u, AM_DEFAULT);
//    DRY_ENUM_ACCESSOR_ATTRIBUTE("Filter Mode", GetFilterMode, SetFilterMode, TextureFilterMode, filterModeNames, FILTER_BILINEAR, AM_DEFAULT); COMPILE ERROR
}

void ReflectionProbe::SetResolution(unsigned res)
{
    resolution_ = ClosestPowerOfTwo(res);
    CreateTextureCube();
}

void ReflectionProbe::SetFilterMode(TextureFilterMode mode)
{
    if (filterMode_ != mode)
    {
        filterMode_ = mode;

        if (reflectionMap_)
            reflectionMap_->SetFilterMode(filterMode_);
    }
}

void ReflectionProbe::OnNodeSet(Node* node)
{
    if (!node)
    {
        if (gimbal_)
            gimbal_->Remove();

        UnsubscribeFromEvent(E_BEGINRENDERING);
        return;
    }

    CreateGimbal();
    CreateViewports();
    CreateTextureCube();

    SubscribeToEvent(E_BEGINRENDERING, DRY_HANDLER(ReflectionProbe, HandleBeginRendering));
}

void ReflectionProbe::CreateGimbal()
{
    if (gimbal_)
    {
        gimbal_->SetParent(node_);
        gimbal_->SetPosition(Vector3::ZERO);
    }
    else
    {
        gimbal_ = node_->CreateChild("RP_GIMBAL");
        gimbal_->SetTemporary(true);
    }

    gimbal_->SetWorldRotation(Quaternion::IDENTITY);
}

void Dry::ReflectionProbe::CreateTextureCube()
{
    if (reflectionMap_)
        reflectionMap_->Release();

    if (resolution_ == 0u)
        return;

    reflectionMap_ = new TextureCube(GetContext());
    reflectionMap_->SetSize(resolution_, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    reflectionMap_->SetFilterMode(filterMode_);

    for (int i{ 0 }; i < viewports_.Size() && i < MAX_CUBEMAP_FACES; ++i)
    {
        const CubeMapFace face{ static_cast<CubeMapFace>(i) };
        RenderSurface* surface{ reflectionMap_->GetRenderSurface(face) };
        surface->SetViewport(0, viewports_.At(i));
        surface->SetUpdateMode(RenderSurfaceUpdateMode::SURFACE_UPDATEALWAYS);
    }
}

void ReflectionProbe::CreateViewports()
{
    for (int i{ 0 }; i < MAX_CUBEMAP_FACES; ++i)
    {
        const CubeMapFace face{ static_cast<CubeMapFace>(i) };
        Node* cameraNode{ gimbal_->CreateChild("RP_CAMERA" + String{ i }) };
        cameraNode->SetRotation(RotationFromFace(face));
        Camera* camera{ cameraNode->CreateComponent<Camera>() };
        camera->SetFov(90.f);
        camera->SetNearClip(nearClip_);
        camera->SetFarClip(farClip_);
        camera->SetViewMask(viewMask_);
        cameras_.Push(camera);

        viewports_.Push(new Viewport(GetContext(), GetScene(), camera));
        if (i != 0)
            viewports_.Back()->SetRenderPath(viewports_.Front()->GetRenderPath());
    }
}

Quaternion ReflectionProbe::RotationFromFace(CubeMapFace face)
{
    switch (face) {
    case FACE_POSITIVE_X: return {  90.f, Vector3::UP };
    case FACE_NEGATIVE_X: return { -90.f, Vector3::UP };
    case FACE_POSITIVE_Y: return { -90.f, Vector3::RIGHT };
    case FACE_NEGATIVE_Y: return {  90.f, Vector3::RIGHT };
    default:              return Quaternion::IDENTITY;
    case FACE_NEGATIVE_Z: return { 180.f, Vector3::UP };
    }
}

void ReflectionProbe::HandleBeginRendering(StringHash eventType, VariantMap& eventData)
{
    if (!gimbal_)
        return;

    gimbal_->SetWorldRotation(Quaternion::IDENTITY);
}

}
