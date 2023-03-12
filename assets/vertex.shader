layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexTextureCoordinates;
layout (location = 3) in ivec4 vertexBoneIds;
layout (location = 4) in vec4 vertexWeights;

const int LAKOT_MAX_BONE_COUNT = 500;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 bones[LAKOT_MAX_BONE_COUNT];

void main() {
    /*
    vec4 position = bones[vertexBoneIds[0]] * vec4(vertexPosition, 1.0f) * vertexWeights[0];
        position += bones[vertexBoneIds[1]] * vec4(vertexPosition, 1.0f) * vertexWeights[1];
        position += bones[vertexBoneIds[2]] * vec4(vertexPosition, 1.0f) * vertexWeights[2];
        position += bones[vertexBoneIds[3]] * vec4(vertexPosition, 1.0f) * vertexWeights[3];
    */
    //if ()

    gl_Position = projection * view * model * vec4(vertexPosition, 1.0f);
}
