#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Fog.glsl"

varying vec2 vTexCoord;
varying vec4 vWorldPos;
varying vec3 vNormal;
varying highp vec4 vEyeVec;
uniform float cFresnelPower;
#ifdef VERTEXCOLOR
    varying vec4 vColor;
#endif

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vNormal = GetWorldNormal(modelMatrix);
    vTexCoord = GetTexCoord(iTexCoord);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vec3 vReflectionVec = worldPos - cCameraPos;
    vEyeVec = vec4(cCameraPos - worldPos, GetDepth(gl_Position));
    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif

}

void PS()
{
    // Get material diffuse albedo
    #ifdef DIFFMAP

        float fireTime = 1.25 * cElapsedTimePS - (vWorldPos.x + vWorldPos.y) * .25;

        float fresnel = pow(clamp(dot(normalize(vEyeVec.xyz), vNormal), 0.0, 1.0), 1.);
        vec4 fire = vec4(texture2D(sDiffMap, vec2(.2, .3) * vTexCoord.xy + fireTime * vec2(0.01, 0.01)).r, 0, 0, 1);
        fire += vec4(1., fire.r, 0., 0.) * texture2D(sDiffMap, vec2(.05, .23) * vTexCoord.xy + fireTime * vec2(-0.05, 0.02)).g;
        float blue = texture2D(sDiffMap, vec2(.2, .1) * vTexCoord.xy + fireTime * vec2(-0.05, 0.02)).b;
        fire *= blue * max(fire.r, fire.g);
        fire += vec4(.5 - fire.g * fire.g, .0, 0.1 * fire.r * fire.g * fire.r * fire.g, fire.r * fire.g);
        fire = min(vec4(1., 1., 1., 1.), fire);

        fresnel *= fresnel + (0.5 + 0.5 * fresnel);
        float fade = (0.5 + 0.5 * rgb2hsv(fire.rgb).b) * fresnel;
        fire.a *= (fade * fade * fire.a) / fresnel;
        fire.b = min(fire.b, fire.a * fire.a);
        fire.g = min(fire.g, fire.a);

        vec4 diffColor = fire;//cMatDiffColor * texture2D(sDiffMap, vTexCoord);
        #ifdef ALPHAMASK
            if (diffColor.a < 0.5)
                discard;
        #endif
    #else
        vec4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= vColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    #if defined(PREPASS)
        // Fill light pre-pass G-Buffer
        gl_FragData[0] = vec4(0.5, 0.5, 0.5, 1.0);
        gl_FragData[1] = vec4(EncodeDepth(vWorldPos.w), 0.0);
    #elif defined(DEFERRED)
        gl_FragData[0] = vec4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
        gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        gl_FragData[2] = vec4(0.5, 0.5, 0.5, 1.0);
        gl_FragData[3] = vec4(EncodeDepth(vWorldPos.w), 0.0);
    #else
        gl_FragColor = vec4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
    #endif
}
