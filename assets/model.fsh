out vec4 FragColor;

in vec2 TextureCoordinates;

uniform sampler2D diffuseTexture;
uniform sampler2D normalsTexture;
uniform sampler2D specularTexture;
uniform sampler2D emissiveTexture;
uniform sampler2D ambientTexture;
uniform sampler2D metalnessTexture;

void main() {
    //FragColor = vec4(0.1f, 0.6f, 0.4f, 1.0f);
    FragColor = texture(diffuseTexture, TextureCoordinates);
}
