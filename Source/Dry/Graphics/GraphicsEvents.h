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

#include "../Core/Object.h"

namespace Dry
{

/// New screen mode set.
DRY_EVENT(E_SCREENMODE, ScreenMode)
{
    DRY_PARAM(P_WIDTH, Width);                  // int
    DRY_PARAM(P_HEIGHT, Height);                // int
    DRY_PARAM(P_FULLSCREEN, Fullscreen);        // bool
    DRY_PARAM(P_BORDERLESS, Borderless);        // bool
    DRY_PARAM(P_RESIZABLE, Resizable);          // bool
    DRY_PARAM(P_HIGHDPI, HighDPI);              // bool
    DRY_PARAM(P_MONITOR, Monitor);              // int
    DRY_PARAM(P_REFRESHRATE, RefreshRate);      // int
}

/// Window position changed.
DRY_EVENT(E_WINDOWPOS, WindowPos)
{
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
}

/// Request for queuing rendersurfaces either in manual or always-update mode.
DRY_EVENT(E_RENDERSURFACEUPDATE, RenderSurfaceUpdate)
{
}

/// Frame rendering started.
DRY_EVENT(E_BEGINRENDERING, BeginRendering)
{
}

/// Frame rendering ended.
DRY_EVENT(E_ENDRENDERING, EndRendering)
{
}

/// Update of a view started.
DRY_EVENT(E_BEGINVIEWUPDATE, BeginViewUpdate)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Update of a view ended.
DRY_EVENT(E_ENDVIEWUPDATE, EndViewUpdate)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of a view started.
DRY_EVENT(E_BEGINVIEWRENDER, BeginViewRender)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// A view has allocated its screen buffers for rendering. They can be accessed now with View::FindNamedTexture().
DRY_EVENT(E_VIEWBUFFERSREADY, ViewBuffersReady)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// A view has set global shader parameters for a new combination of vertex/pixel shaders. Custom global parameters can now be set.
DRY_EVENT(E_VIEWGLOBALSHADERPARAMETERS, ViewGlobalShaderParameters)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of a view ended. Its screen buffers are still accessible if needed.
DRY_EVENT(E_ENDVIEWRENDER, EndViewRender)
{
    DRY_PARAM(P_VIEW, View);                    // View pointer
    DRY_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DRY_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DRY_PARAM(P_SCENE, Scene);                  // Scene pointer
    DRY_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of all views is finished for the frame.
DRY_EVENT(E_ENDALLVIEWSRENDER, EndAllViewsRender)
{
}

/// A render path event has occurred.
DRY_EVENT(E_RENDERPATHEVENT, RenderPathEvent)
{
    DRY_PARAM(P_NAME, Name);                    // String
}

/// Graphics context has been lost. Some or all (depending on the API) GPU objects have lost their contents.
DRY_EVENT(E_DEVICELOST, DeviceLost)
{
}

/// Graphics context has been recreated after being lost. GPU objects in the "data lost" state can be restored now.
DRY_EVENT(E_DEVICERESET, DeviceReset)
{
}

}
