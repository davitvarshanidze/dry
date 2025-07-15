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

/// \file

#pragma once

#include "../Scene/Component.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"

namespace Dry
{

/// %Reflection probe component.
class DRY_API ReflectionProbe: public Component
{
    DRY_OBJECT(ReflectionProbe, Component)

public:
    /// Construct.
    ReflectionProbe(Context* context);
    /// Destruct.
    ~ReflectionProbe();;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Return reflection environment map.
    TextureCube* GetReflectionMap() const { return reflectionMap_; }

    /// Set texture cube resolution.
    void SetResolution(unsigned res);;
    /// Get texture cube resolution.
    unsigned GetResolution() const { return resolution_; };
    /// Set filtering mode.
    void SetFilterMode(TextureFilterMode mode);
    /// Return filtering mode.
    TextureFilterMode GetFilterMode() const { return filterMode_; }

    /// Set near clip on all cameras.
    void SetNearClip(float clip)
    {
        nearClip_ = clip;
        for (Camera* c: cameras_)
            c->SetNearClip(nearClip_);
    }
    /// Return near clip.
    float GetNearClip() const { return nearClip_; }
    /// Set far clip on all cameras.
    void SetFarClip(float clip)
    {
        farClip_ = clip;
        for (Camera* c: cameras_)
            c->SetFarClip(farClip_);
    }
    /// Return far clip.
    float GetFarClip() const { return farClip_; }
    /// Set view mask of all cameras.
    void SetViewMask(unsigned mask)
    {
        viewMask_ = mask;
        for (Camera* c: cameras_)
            c->SetViewMask(viewMask_);
    }
    /// Return view mask.
    unsigned GetViewMask() const { return viewMask_; }

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* node) override;

private:
    /// Returns the required rotation for a camera, based on a cube map face.
    static Quaternion RotationFromFace(CubeMapFace face);

    /// Create gimbal.
    void CreateGimbal();
    /// Create texture cube.
    void CreateTextureCube();
    /// Create viewports.
    void CreateViewports();
    /// Realign gimbal with world.
    void HandleBeginRendering(StringHash eventType, VariantMap& eventData);

    /// Child node that keeps the cameras world-aligned.
    Node* gimbal_;
    /// Viewports of all texture cube faces.
    PODVector<Viewport*> viewports_;
    /// Cameras of all texture cube faces.
    PODVector<Camera*> cameras_;

    /// Shared near clip.
    float nearClip_;
    /// Shared far clip.
    float farClip_;
    /// Shared view mask.
    unsigned viewMask_;
    /// Texture cube resolution.
    unsigned resolution_;
    /// Texture cube filter mode.
    TextureFilterMode filterMode_;
    /// Render path reused for all viewports.
    SharedPtr<RenderPath> renderPath_;
    /// Reflection texture cube.
    TextureCube* reflectionMap_;
};

}
