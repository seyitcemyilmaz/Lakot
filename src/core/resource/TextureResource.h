#ifndef LAKOT_TEXTURERESOURCE_H
#define LAKOT_TEXTURERESOURCE_H

#include <string>

enum class TextureType {
    eNoneTexture,
    eDiffuseTexture,
    eSpecularTexture,
    eAmbientTexture,
    eEmissiveTexture,
    eHeightTexture,
    eNormalsTexture,
    eShininessTexture,
    eOpacityTexture,
    eDisplacementTexture,
    eLightmapTexture,
    eReflectionTexture,
    eBaseColorTexture,
    eNormalCameraTexture,
    eEmissionColorTexture,
    eMetalnessTexture,
    eDiffuseRoughnessTexture,
    eAmbientOcclusionTexture,
    eSheenTexture,
    eClearcoatTexture,
    eTransmissionTexture,
    eUnknownTexture
};

class TextureResource {
private:
	unsigned int mTextureId;
    std::string mTexturePath;

public:
    TextureResource(unsigned int pTextureId, const std::string& pTexturePath);

	unsigned int getTextureId() const;
    const std::string& getTexturePath() const;
};

#endif
