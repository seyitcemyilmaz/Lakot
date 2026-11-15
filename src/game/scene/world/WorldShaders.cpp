#include "WorldShaders.h"

#include <string>

#include "../../graphics/geometry/TerrainShaders.h"
#include "../../graphics/render/ShaderManager.h"

using namespace lakot;

void WorldShaders::ensurePrograms(ShaderManager& pShaderManager)
{
    const std::string tFogFunction = R"(
        uniform vec3 uCameraPosition;
        uniform vec3 uFogColor;
        uniform float uFogStart;
        uniform float uFogEnd;

        vec3 applyFog(vec3 pColor, vec3 pWorldPosition)
        {
            float tDistance = distance(pWorldPosition, uCameraPosition);
            float tFog = clamp((tDistance - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
            return mix(pColor, uFogColor, tFog);
        }
    )";

    const std::string tLightFunction = R"(
        uniform vec3 uLightDirection;

        vec3 applyLight(vec3 pColor, vec3 pNormal)
        {
            float tDiffuse = max(dot(normalize(pNormal), -uLightDirection), 0.0);
            return pColor * (0.52 + 0.62 * tDiffuse);
        }
    )";

    std::string tBoxInstancedVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aOffset;
        layout (location = 2) in vec3 aSize;
        layout (location = 3) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vColor;
        out vec3 vWorldPosition;

        void main()
        {
            vWorldPosition = (aPos * aSize) + aOffset;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
            vColor = aColor;
        }
    )";

    std::string tBoxInstancedFragmentSource = R"(
        in vec3 vColor;
        in vec3 vWorldPosition;
        out vec4 FragColor;
    )" + tFogFunction + tLightFunction + R"(
        void main()
        {
            vec3 tNormal = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
            FragColor = vec4(applyFog(applyLight(vColor, tNormal), vWorldPosition), 1.0);
        }
    )";

    std::string tModelVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUv;
        layout (location = 3) in vec3 aOffset;
        layout (location = 4) in vec3 aYawScaleStretch;

        uniform mat4 uViewProjection;

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec2 vUv;

        void main()
        {
            float tCos = cos(aYawScaleStretch.x);
            float tSin = sin(aYawScaleStretch.x);
            vec3 tScale = vec3(aYawScaleStretch.y * aYawScaleStretch.z, aYawScaleStretch.y, aYawScaleStretch.y);
            mat3 tRotation = mat3(tCos, 0.0, -tSin,
                                  0.0,  1.0,  0.0,
                                  tSin, 0.0,  tCos);

            vWorldPosition = tRotation * (aPos * tScale) + aOffset;
            vNormal = tRotation * aNormal;
            vUv = aUv;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
        }
    )";

    const std::string tModelFragmentHead = R"(
        in vec3 vWorldPosition;
        in vec3 vNormal;
        in vec2 vUv;
        out vec4 FragColor;

        uniform sampler2D uTexture;
        uniform vec3 uTint = vec3(1.0);
        uniform float uGlow = 0.0;
    )" + tFogFunction + tLightFunction;

    const std::string tModelFragmentBody = R"(
        void main()
        {
            float tCameraDistance = distance(vWorldPosition, uCameraPosition);

            if (kFadeNearCamera && tCameraDistance < 2.4)
            {
                float tNoise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));

                if (smoothstep(0.8, 2.4, tCameraDistance) <= tNoise)
                {
                    discard;
                }
            }

            vec4 tSample = texture(uTexture, vUv);
            vec2 tTexels = vUv * vec2(textureSize(uTexture, 0));
            float tLod = max(0.0, log2(max(max(length(dFdx(tTexels)), length(dFdy(tTexels))), 1e-6)));
            float tAlpha = tSample.a * (1.0 + 0.3 * tLod);

            if (tAlpha < 0.5)
            {
                discard;
            }

            vec3 tColor = tSample.rgb * uTint;
            vec3 tNormal = gl_FrontFacing ? vNormal : -vNormal;
            FragColor = vec4(applyFog(applyLight(tColor, tNormal) + tColor * uGlow, vWorldPosition), 1.0);
        }
    )";

    const std::string tModelFragmentSource = tModelFragmentHead + "const bool kFadeNearCamera = true;\n" + tModelFragmentBody;
    const std::string tSkinnedFragmentSource = tModelFragmentHead + "const bool kFadeNearCamera = false;\n" + tModelFragmentBody;

    std::string tSkinnedVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUv;
        layout (location = 3) in ivec4 aJoints;
        layout (location = 4) in vec4 aWeights;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;
        uniform mat4 uBones[64];

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec2 vUv;

        void main()
        {
            mat4 tSkin = uBones[aJoints.x] * aWeights.x + uBones[aJoints.y] * aWeights.y
                       + uBones[aJoints.z] * aWeights.z + uBones[aJoints.w] * aWeights.w;
            mat4 tWorld = uModel * tSkin;

            vWorldPosition = (tWorld * vec4(aPos, 1.0)).xyz;
            vNormal = mat3(tWorld) * aNormal;
            vUv = aUv;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
        }
    )";

    if (!pShaderManager.hasProgram("world_box"))
    {
        pShaderManager.createProgram("world_box", tBoxInstancedVertexSource, tBoxInstancedFragmentSource);
    }

    if (!pShaderManager.hasProgram("world_terrain"))
    {
        pShaderManager.createProgram("world_terrain", TerrainShaders::vertexSource(), TerrainShaders::fragmentSource(tFogFunction + tLightFunction));
    }

    if (!pShaderManager.hasProgram("world_model"))
    {
        pShaderManager.createProgram("world_model", tModelVertexSource, tModelFragmentSource);
    }

    if (!pShaderManager.hasProgram("world_skinned"))
    {
        pShaderManager.createProgram("world_skinned", tSkinnedVertexSource, tSkinnedFragmentSource);
    }
}
