out vec4 FragColor;

in vec2 TextureCoordinates;

struct Material {
    sampler2D diffuseTexture;
    sampler2D normalsTexture;
    sampler2D specularTexture;
    sampler2D emissiveTexture;
    sampler2D ambientTexture;
    sampler2D metalnessTexture;

    bool hasDiffuseTexture;
    bool hasNormalsTexture;
    bool hasSpecularTexture;
    bool hasEmissiveTexture;
    bool hasAmbientTexture;
    bool hasMetalnessTexture;

    vec3 diffuseColor;
    vec3 specularColor;
    vec3 emissiveColor;
    vec3 ambientColor;
};

uniform Material material;

void main() {
    if (material.hasDiffuseTexture) {
	FragColor = texture(material.diffuseTexture, TextureCoordinates);
    }
    else {
	FragColor = vec4(material.diffuseColor, 1.0f);
    }

    if (material.hasMetalnessTexture) {
	FragColor *= texture(material.metalnessTexture, TextureCoordinates);
    }
}
