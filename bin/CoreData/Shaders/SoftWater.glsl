#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

#ifndef GL_ES
varying vec4 vScreenPos;
varying vec2 vReflectUV;
varying vec2 vWaterUV;
varying vec4 vEyeVec;
#else
varying highp vec4 vScreenPos;
varying highp vec2 vReflectUV;
varying highp vec2 vWaterUV;
varying highp vec4 vEyeVec;
#endif
varying vec3 vNormal;
varying vec4 vWorldPos;

#ifdef PERPIXEL
//    #ifdef SHADOW
//        #ifndef GL_ES
//            varying vec4 vShadowPos[NUMCASCADES];
//        #else
//            varying highp vec4 vShadowPos[NUMCASCADES];
//        #endif
//    #endif
    #ifdef SPOTLIGHT
        varying vec4 vSpotPos;
    #endif
    #ifdef POINTLIGHT
        varying vec3 vCubeMaskVec;
    #endif
#else
    varying vec3 vVertexLight;
    #ifdef ENVCUBEMAP
        varying vec3 vReflectionVec;
    #endif
    #if defined(LIGHTMAP) || defined(AO)
        varying vec2 vTexCoord2;
    #endif
#endif

#ifdef COMPILEVS
uniform vec2 cNoiseSpeed;
uniform float cNoiseTiling;
#endif
#ifdef COMPILEPS
uniform float cNoiseStrength;
uniform float cFresnelPower;
uniform vec3 cWaterTint;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vScreenPos = GetScreenPos(gl_Position);
    // GetQuadTexCoord() returns a vec2 that is OK for quad rendering; multiply it with output W
    // coordinate to make it work with arbitrary meshes such as the water plane (perform divide in pixel shader)
    // Also because the quadTexCoord is based on the clip position, and Y is flipped when rendering to a texture
    // on OpenGL, must flip again to cancel it out
    vReflectUV = GetQuadTexCoord(gl_Position);
    vReflectUV.y = 1.0 - vReflectUV.y;
    vReflectUV *= gl_Position.w;
    vWaterUV = iTexCoord * cNoiseTiling + cElapsedTime * cNoiseSpeed;
    vNormal = GetWorldNormal(modelMatrix);
    vEyeVec = vec4(cCameraPos - worldPos, GetDepth(gl_Position));

//#ifdef SPECULAR
//#ifdef PERPIXEL
//#ifdef SHADOW
//    // Per-pixel forward lighting
//    vec4 projWorldPos = vec4(vWorldPos.xyz, 1.0);
//
//    #ifdef SHADOW
//        // Shadow projection: transform from world space to shadow space
//        for (int i = 0; i < NUMCASCADES; i++)
//            vShadowPos[i] = GetShadowPos(i, vNormal, projWorldPos);
//    #endif
//#endif
//#endif
//#endif
}

void PS()
{
#ifdef SOFT
//    float fadeScale = 10;
//    float surfaceDepth = vWorldPos.w;
//    #ifdef HWDEPTH
//        float depth = ReconstructDepth(texture2DProj(sDepthBuffer, vScreenPos).r);
//    #else
//        float depth = DecodeDepth(texture2DProj(sDepthBuffer, vScreenPos).rgb);
//    #endif
//
//    #ifdef EXPAND
//        float diffZ = max(surfaceDepth - depth, 0.0) * (cFarClipPS - cNearClipPS);
//        float fade = clamp(diffZ * fadeScale, 0.0, 1.0);
//    #else
//        float diffZ = (depth - surfaceDepth) * (cFarClipPS - cNearClipPS);
//        float fade = clamp(1.0 - diffZ * fadeScale, 0.0, 1.0);
//    #endif
//
//
#endif
    vec2 refractUV = vScreenPos.xy / vScreenPos.w;
    vec2 reflectUV = vReflectUV.xy / vScreenPos.w;

    float y = cNoiseStrength * 2 * texture2D(sNormalMap, vec2( -.125, -.123) * vWaterUV).b - 1;
    vec2 noiseNormal = texture2D(sNormalMap, vWaterUV).rg;

//    float a = (cElapsedTimePS * y + length(vWorldPos.xy) * .1) * (5 + noiseNormal.x * .1);
//	float s = sin(a * y);
//	float c = cos(a * y);
//	mat2 m = mat2(c, s, -s, c);
//	vec2 sway =  m * noiseNormal * cNoiseStrength * 2;
//
//    noiseNormal.x *= y * y + sway.x;
//    noiseNormal.y *= -y * ((1.666 + sway.y) + y * noiseNormal.x);
    noiseNormal.x *= y * y;
    noiseNormal.y *= -y * (1.666 + y * noiseNormal.x);


    vec2 noise = (noiseNormal - 0.5) * cNoiseStrength;
    vec2 refractUV2 = refractUV + noise * vec2(-1, 1);
    refractUV += noise;

    // Do not shift reflect UV coordinate upward, because it will reveal the clipping of geometry below water
    if (noise.y < 0.0)
        noise.y = 0.0;
    else
        noise.y *= 1.25;

    reflectUV += noise;

    float fresnel = pow(1.0 - clamp(dot(normalize(vEyeVec.xyz), vNormal), 0.0, .5), cFresnelPower * (1 + length(noise) * 5 + noiseNormal.y * .333));
    vec3 refractColor = mix(mix(texture2D(sEnvMap, refractUV).rgb, texture2D(sEnvMap, refractUV2).rgb, noiseNormal.y) * cWaterTint, cWaterTint, min(1, GetFogFactor(vWorldPos.w) * .25));
    vec3 reflectColor = texture2D(sDiffMap, reflectUV).rgb;
    vec3 finalColor = mix(refractColor, reflectColor, fresnel);

#ifdef SPECULAR
    #ifdef PERPIXEL


        // Get material specular albedo
        #ifdef SPECMAP
            vec3 specColor = cMatSpecColor.rgb * texture2D(sSpecMap, vTexCoord.xy).rgb;
        #else
            vec3 specColor = cMatSpecColor.rgb;
        #endif

        // Per-pixel forward lighting
        vec3 lightColor;
        vec3 lightDir;

        #if defined(SPOTLIGHT)
            lightColor = vSpotPos.w > 0.0 ? texture2DProj(sLightSpotMap, vSpotPos).rgb * cLightColor.rgb : vec3(0.0, 0.0, 0.0);
        #elif defined(CUBEMASK)
            lightColor = textureCube(sLightCubeMap, vCubeMaskVec).rgb * cLightColor.rgb;
        #else
            lightColor = cLightColor.rgb;
        #endif

        y *= y;
        vec3 normal = normalize(vNormal + vec3(noiseNormal.x * abs(y), y, noiseNormal.y * abs(y)));

        GetDiffuse(normal, vWorldPos.xyz, lightDir);

        float smear = 17;

        float spec = GetSpecular(normal, cCameraPosPS - vWorldPos.xyz, lightDir, cMatSpecColor.a / smear);
//        #ifdef SHADOW
//            spec *= GetShadow(vShadowPos, vWorldPos.w);;
//        #endif

        if (spec < .9992)
        {
            spec *= 0.3;
            spec = pow(spec, 23 / smear);
        }
        else if (spec < .9997)
            spec *= 0.5;

        gl_FragColor = spec * vec4(mix(lightColor, cMatSpecColor.rgb, spec < .9999?1:.5*spec), 1);
    #endif
#else
    gl_FragColor = vec4(finalColor, .9);
#endif
}
