#include "TerrainShaders.h"

using namespace lakot;

std::string TerrainShaders::vertexSource()
{
    return R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec3 vColor;

        void main()
        {
            vWorldPosition = aPos;
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProjection * vec4(aPos, 1.0);
        }
    )";
}

std::string TerrainShaders::fragmentSource(const std::string& pSharedFunctions)
{
    return R"(
        in vec3 vWorldPosition;
        in vec3 vNormal;
        in vec3 vColor;
        out vec4 FragColor;

        uniform int uUseDetail = 0;
        uniform float uWorldSize = 2048.0;
        uniform sampler2D uSplat;
        uniform sampler2D uGrass;
        uniform sampler2D uDryGrass;
        uniform sampler2D uDirt;
        uniform sampler2D uRoad;
        uniform sampler2D uRock;
        uniform sampler2D uMacro;

        const float kNearScale = 1.0 / 6.0;
        const float kFarScale = 1.0 / 37.0;
        const float kMacroScale = 1.0 / 160.0;
        const float kEdgeScale = 1.0 / 23.0;
        const float kFadeStart = 55.0;
        const float kFadeEnd = 110.0;
        const float kMinWeight = 0.02;
        const mat2 kRotation = mat2(0.8, 0.6, -0.6, 0.8);
    )" + pSharedFunctions + R"(
        vec3 sampleLayer(sampler2D pLayer, vec2 pNear, vec2 pNearDx, vec2 pNearDy, vec2 pFar, vec2 pFarDx, vec2 pFarDy, float pBlend, float pFade)
        {
            vec3 tAverage = textureLod(pLayer, vec2(0.5), 16.0).rgb;

            if (pFade >= 1.0)
            {
                return tAverage;
            }

            vec3 tNear = textureGrad(pLayer, pNear, pNearDx, pNearDy).rgb;
            vec3 tFar = textureGrad(pLayer, pFar, pFarDx, pFarDy).rgb;
            return mix(mix(tNear, tFar, pBlend), tAverage, pFade);
        }

        vec3 groundDetail()
        {
            vec2 tWorld = vWorldPosition.xz;
            vec2 tSplatUv = vec2(tWorld.x / uWorldSize + 0.5, 0.5 - tWorld.y / uWorldSize);
            vec4 tSplat = texture(uSplat, tSplatUv);

            float tMacro = texture(uMacro, tWorld * kMacroScale).r;
            float tEdge = texture(uMacro, kRotation * tWorld * kEdgeScale + vec2(0.37, 0.61)).r;

            float tSlope = 1.0 - clamp(normalize(vNormal).y, 0.0, 1.0);
            float tRockSlope = smoothstep(0.2, 0.42, tSlope);

            float tEdgeShift = (tEdge - 0.5) * 0.35;
            float tDry = smoothstep(0.15, 0.85, tSplat.r + tEdgeShift);
            float tDirt = smoothstep(0.15, 0.85, tSplat.g + tEdgeShift);
            float tRoad = smoothstep(0.15, 0.85, tSplat.b + tEdgeShift);
            float tRock = max(smoothstep(0.15, 0.85, tSplat.a + tEdgeShift), tRockSlope);
            float tGrass = max(1.0 - (tDry + tDirt + tRoad + tRock), 0.0);

            float tScale = 1.0 / (tGrass + tDry + tDirt + tRoad + tRock);
            tGrass *= tScale;
            tDry *= tScale;
            tDirt *= tScale;
            tRoad *= tScale;
            tRock *= tScale;

            float tDistance = distance(vWorldPosition, uCameraPosition);
            float tFade = smoothstep(kFadeStart, kFadeEnd, tDistance);
            float tBlend = clamp(0.25 + tMacro * 0.7, 0.0, 1.0);

            vec2 tNear = tWorld * kNearScale;
            vec2 tNearDx = dFdx(tWorld) * kNearScale;
            vec2 tNearDy = dFdy(tWorld) * kNearScale;
            vec2 tFar = kRotation * tWorld * kFarScale;
            vec2 tFarDx = kRotation * dFdx(tWorld) * kFarScale;
            vec2 tFarDy = kRotation * dFdy(tWorld) * kFarScale;

            vec3 tColor = vec3(0.0);

            if (tGrass > kMinWeight)
            {
                tColor += tGrass * sampleLayer(uGrass, tNear, tNearDx, tNearDy, tFar, tFarDx, tFarDy, tBlend, tFade);
            }

            if (tDry > kMinWeight)
            {
                tColor += tDry * sampleLayer(uDryGrass, tNear, tNearDx, tNearDy, tFar, tFarDx, tFarDy, tBlend, tFade);
            }

            if (tDirt > kMinWeight)
            {
                tColor += tDirt * sampleLayer(uDirt, tNear, tNearDx, tNearDy, tFar, tFarDx, tFarDy, tBlend, tFade);
            }

            if (tRoad > kMinWeight)
            {
                tColor += tRoad * sampleLayer(uRoad, tNear, tNearDx, tNearDy, tFar, tFarDx, tFarDy, tBlend, tFade);
            }

            if (tRock > kMinWeight)
            {
                tColor += tRock * sampleLayer(uRock, tNear, tNearDx, tNearDy, tFar, tFarDx, tFarDy, tBlend, tFade);
            }

            float tMacroTone = 0.85 + 0.3 * tMacro;
            return mix(tColor * tMacroTone, tColor * vColor * 2.2, 0.35);
        }

        void main()
        {
            vec3 tBase = uUseDetail != 0 ? groundDetail() : vColor;
            FragColor = vec4(applyFog(applyLight(tBase, vNormal), vWorldPosition), 1.0);
        }
    )";
}
