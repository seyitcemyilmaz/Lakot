#ifndef LAKOT_SHADERSOURCE_H
#define LAKOT_SHADERSOURCE_H

namespace lakot {

static const char* BOX_VERTEX_SHADER = R"(
layout (location = 0) in vec3 staticPosition;
layout (location = 1) in vec3 dynamicPosition;
layout (location = 2) in vec3 dynamicSize;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    vec3 tPosition = staticPosition * dynamicSize;
    gl_Position = projection * view * vec4((tPosition + dynamicPosition), 1.0);
}
)";

static const char* BOX_FRAGMENT_SHADER = R"(
out vec4 fColor;

void main()
{
    fColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}
)";

static const char* MESH_VERTEX_SHADER = R"(
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textureCoordinates;
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 boneWeights;

out vec4 vPosition;
out vec3 vNormal;
out vec2 vTextureCoordinates;

const int LAKOT_MAX_BONE_COUNT = 64;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// uniform mat4 boneTransformations[LAKOT_MAX_BONE_COUNT];

void main()
{
    vPosition = model * vec4(position, 1.0);
    vTextureCoordinates = textureCoordinates;
    vNormal = mat3(transpose(inverse(model))) * normal;

    gl_Position = projection * view * vPosition;
}
)";

static const char* MESH_FRAGMENT_SHADER = R"(
out vec4 fColor;

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTextureCoordinates;

void main()
{
    fColor = vec4(1.0, 1.0, 0.0, 1.0);
}
)";

}

#endif
