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
};

uniform Material material;

void main() {
    if (material.hasNormalsTexture) {
        FragColor = texture(material.normalsTexture, TextureCoordinates);
    }
    else {
        FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
}
