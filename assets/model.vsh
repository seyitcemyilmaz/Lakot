layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinates;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aBoneWeights;

out vec2 TextureCoordinates;

const int LAKOT_MAX_BONE_COUNT = 256;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 bones[LAKOT_MAX_BONE_COUNT];

void main() {
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);
    TextureCoordinates = aTextureCoordinates;
}
