layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinates;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aBoneWeights;

out vec4 FragPos;
out vec3 Normal;
out vec2 TextureCoordinates;

const int LAKOT_MAX_BONE_COUNT = 128;
const vec4 ZERO_VECTOR = vec4(0.0f);

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 boneTransformations[LAKOT_MAX_BONE_COUNT];

uniform int animationType;

void main() {
    TextureCoordinates = aTextureCoordinates;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    if (animationType == 0) {
	FragPos = model * vec4(aPosition, 1.0f);
    }
    else if (animationType == 1) {
	vec4 totalPosition = vec4(0.0f);

	totalPosition += boneTransformations[aBoneIds[0]] * vec4(aPosition, 1.0f) * aBoneWeights[0];
	totalPosition += boneTransformations[aBoneIds[1]] * vec4(aPosition, 1.0f) * aBoneWeights[1];
	totalPosition += boneTransformations[aBoneIds[2]] * vec4(aPosition, 1.0f) * aBoneWeights[2];
	totalPosition += boneTransformations[aBoneIds[3]] * vec4(aPosition, 1.0f) * aBoneWeights[3];

	if (any(notEqual(totalPosition, ZERO_VECTOR))) {
	    FragPos = model * totalPosition;
	}
	else {
	    FragPos = model * vec4(aPosition, 1.0f);
	}
    }

    gl_Position = projection * view * FragPos;
}
