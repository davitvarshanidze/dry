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

#include "../Graphics/GraphicsDefs.h"
#include "../Math/Vector3.h"

#include "../DebugNew.h"

namespace Dry
{

// The extern keyword is required when building Dry.dll for Windows platform
// The keyword is not required for other platforms but it does no harm, aside from warning from static analyzer

extern DRY_API const StringHash VSP_AMBIENTSTARTCOLOR("AmbientStartColor");
extern DRY_API const StringHash VSP_AMBIENTENDCOLOR("AmbientEndColor");
extern DRY_API const StringHash VSP_BILLBOARDROT("BillboardRot");
extern DRY_API const StringHash VSP_CAMERAPOS("CameraPos");
extern DRY_API const StringHash VSP_CLIPPLANE("ClipPlane");
extern DRY_API const StringHash VSP_NEARCLIP("NearClip");
extern DRY_API const StringHash VSP_FARCLIP("FarClip");
extern DRY_API const StringHash VSP_DEPTHMODE("DepthMode");
extern DRY_API const StringHash VSP_DELTATIME("DeltaTime");
extern DRY_API const StringHash VSP_ELAPSEDTIME("ElapsedTime");
extern DRY_API const StringHash VSP_FRUSTUMSIZE("FrustumSize");
extern DRY_API const StringHash VSP_GBUFFEROFFSETS("GBufferOffsets");
extern DRY_API const StringHash VSP_LIGHTDIR("LightDir");
extern DRY_API const StringHash VSP_LIGHTPOS("LightPos");
extern DRY_API const StringHash VSP_NORMALOFFSETSCALE("NormalOffsetScale");
extern DRY_API const StringHash VSP_MODEL("Model");
extern DRY_API const StringHash VSP_VIEW("View");
extern DRY_API const StringHash VSP_VIEWINV("ViewInv");
extern DRY_API const StringHash VSP_VIEWPROJ("ViewProj");
extern DRY_API const StringHash VSP_UOFFSET("UOffset");
extern DRY_API const StringHash VSP_VOFFSET("VOffset");
extern DRY_API const StringHash VSP_ZONE("Zone");
extern DRY_API const StringHash VSP_LIGHTMATRICES("LightMatrices");
extern DRY_API const StringHash VSP_SKINMATRICES("SkinMatrices");
extern DRY_API const StringHash VSP_VERTEXLIGHTS("VertexLights");
extern DRY_API const StringHash PSP_AMBIENTCOLOR("AmbientColor");
extern DRY_API const StringHash PSP_CAMERAPOS("CameraPosPS");
extern DRY_API const StringHash PSP_DELTATIME("DeltaTimePS");
extern DRY_API const StringHash PSP_DEPTHRECONSTRUCT("DepthReconstruct");
extern DRY_API const StringHash PSP_ELAPSEDTIME("ElapsedTimePS");
extern DRY_API const StringHash PSP_FOGCOLOR("FogColor");
extern DRY_API const StringHash PSP_FOGPARAMS("FogParams");
extern DRY_API const StringHash PSP_GBUFFERINVSIZE("GBufferInvSize");
extern DRY_API const StringHash PSP_LIGHTCOLOR("LightColor");
extern DRY_API const StringHash PSP_LIGHTDIR("LightDirPS");
extern DRY_API const StringHash PSP_LIGHTPOS("LightPosPS");
extern DRY_API const StringHash PSP_NORMALOFFSETSCALE("NormalOffsetScalePS");
extern DRY_API const StringHash PSP_MATDIFFCOLOR("MatDiffColor");
extern DRY_API const StringHash PSP_MATEMISSIVECOLOR("MatEmissiveColor");
extern DRY_API const StringHash PSP_MATENVMAPCOLOR("MatEnvMapColor");
extern DRY_API const StringHash PSP_MATSPECCOLOR("MatSpecColor");
extern DRY_API const StringHash PSP_NEARCLIP("NearClipPS");
extern DRY_API const StringHash PSP_FARCLIP("FarClipPS");
extern DRY_API const StringHash PSP_SHADOWCUBEADJUST("ShadowCubeAdjust");
extern DRY_API const StringHash PSP_SHADOWDEPTHFADE("ShadowDepthFade");
extern DRY_API const StringHash PSP_SHADOWINTENSITY("ShadowIntensity");
extern DRY_API const StringHash PSP_SHADOWMAPINVSIZE("ShadowMapInvSize");
extern DRY_API const StringHash PSP_SHADOWSPLITS("ShadowSplits");
extern DRY_API const StringHash PSP_LIGHTMATRICES("LightMatricesPS");
extern DRY_API const StringHash PSP_VSMSHADOWPARAMS("VSMShadowParams");
extern DRY_API const StringHash PSP_ROUGHNESS("Roughness");
extern DRY_API const StringHash PSP_METALLIC("Metallic");
extern DRY_API const StringHash PSP_LIGHTRAD("LightRad");
extern DRY_API const StringHash PSP_LIGHTLENGTH("LightLength");
extern DRY_API const StringHash PSP_ZONEMIN("ZoneMin");
extern DRY_API const StringHash PSP_ZONEMAX("ZoneMax");

extern DRY_API const Vector3 DOT_SCALE(1 / 3.0f, 1 / 3.0f, 1 / 3.0f);

extern DRY_API const VertexElement LEGACY_VERTEXELEMENTS[] =
{
    VertexElement(TYPE_VECTOR3, SEM_POSITION, 0, false),     // Position
    VertexElement(TYPE_VECTOR3, SEM_NORMAL, 0, false),       // Normal
    VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, 0, false),    // Color
    VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0, false),     // Texcoord1
    VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 1, false),     // Texcoord2
    VertexElement(TYPE_VECTOR3, SEM_TEXCOORD, 0, false),     // Cubetexcoord1
    VertexElement(TYPE_VECTOR3, SEM_TEXCOORD, 1, false),     // Cubetexcoord2
    VertexElement(TYPE_VECTOR4, SEM_TANGENT, 0, false),      // Tangent
    VertexElement(TYPE_VECTOR4, SEM_BLENDWEIGHTS, 0, false), // Blendweights
    VertexElement(TYPE_UBYTE4, SEM_BLENDINDICES, 0, false),  // Blendindices
    VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 4, true),      // Instancematrix1
    VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 5, true),      // Instancematrix2
    VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 6, true),      // Instancematrix3
    VertexElement(TYPE_INT, SEM_OBJECTINDEX, 0, false)       // Objectindex
};

extern DRY_API const unsigned ELEMENT_TYPESIZES[] =
{
    sizeof(int),
    sizeof(float),
    2 * sizeof(float),
    3 * sizeof(float),
    4 * sizeof(float),
    sizeof(unsigned),
    sizeof(unsigned)
};


}
