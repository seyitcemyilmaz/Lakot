#ifndef MATERIALRESOURCE_H
#define MATERIALRESOURCE_H

#include <glm/glm.hpp>

#include "TextureResource.h"

class MaterialResource
{
private:
    TextureResource* mDiffuseTexture = nullptr;
    TextureResource* mSpecularTexture = nullptr;
    TextureResource* mAmbientTexture = nullptr;
    TextureResource* mEmissiveTexture = nullptr;
    TextureResource* mHeightTexture = nullptr;
    TextureResource* mNormalsTexture = nullptr;
    TextureResource* mShininessTexture = nullptr;
    TextureResource* mOpacityTexture = nullptr;
    TextureResource* mDisplacementTexture = nullptr;
    TextureResource* mLightmapTexture = nullptr;
    TextureResource* mReflectionTexture = nullptr;
    TextureResource* mBaseColorTexture = nullptr;
    TextureResource* mNormalCameraTexture = nullptr;
    TextureResource* mEmissionColorTexture = nullptr;
    TextureResource* mMetalnessTexture = nullptr;
    TextureResource* mDiffuseRoughnessTexture = nullptr;
    TextureResource* mAmbientOcclusionTexture = nullptr;
    TextureResource* mSheenTexture = nullptr;
    TextureResource* mClearcoatTexture = nullptr;
    TextureResource* mTransmissionTexture = nullptr;

    glm::vec3 mDiffuseColor;
    glm::vec3 mSpecularColor;
    glm::vec3 mEmissiveColor;
    glm::vec3 mAmbientColor;

    float mShininess;
    float mOpacity;

protected:
    void setDiffuseTexture(TextureResource* pTextureResource);
    void setSpecularTexture(TextureResource* pTextureResource);
    void setAmbientTexture(TextureResource* pTextureResource);
    void setEmissiveTexture(TextureResource* pTextureResource);
    void setHeightTexture(TextureResource* pTextureResource);
    void setNormalsTexture(TextureResource* pTextureResource);
    void setShininessTexture(TextureResource* pTextureResource);
    void setOpacityTexture(TextureResource* pTextureResource);
    void setDisplacementTexture(TextureResource* pTextureResource);
    void setLightmapTexture(TextureResource* pTextureResource);
    void setReflectionTexture(TextureResource* pTextureResource);
    void setBaseColorTexture(TextureResource* pTextureResource);
    void setNormalCameraTexture(TextureResource* pTextureResource);
    void setEmissionColorTexture(TextureResource* pTextureResource);
    void setMetalnessTexture(TextureResource* pTextureResource);
    void setDiffuseRoughnessTexture(TextureResource* pTextureResource);
    void setAmbientOcclusionTexture(TextureResource* pTextureResource);
    void setSheenTexture(TextureResource* pTextureResource);
    void setClearcoatTexture(TextureResource* pTextureResource);
    void setTransmissionTexture(TextureResource* pTextureResource);

    friend class MaterialLoader;

public:
    MaterialResource(
        const glm::vec3& pDiffuseColor,
        const glm::vec3& pSpecularColor,
        const glm::vec3& pEmissiveColor,
        const glm::vec3& pAmbientColor,
        float pShininess,
        float pOpacity);

    TextureResource* getDiffuseTexture();
    TextureResource* getSpecularTexture();
    TextureResource* getAmbientTexture();
    TextureResource* getEmissiveTexture();
    TextureResource* getHeightTexture();
    TextureResource* getNormalsTexture();
    TextureResource* getShininessTexture();
    TextureResource* getOpacityTexture();
    TextureResource* getDisplacementTexture();
    TextureResource* getLightmapTexture();
    TextureResource* getReflectionTexture();
    TextureResource* getBaseColorTexture();
    TextureResource* getNormalCameraTexture();
    TextureResource* getEmissionColorTexture();
    TextureResource* getMetalnessTexture();
    TextureResource* getDiffuseRoughnessTexture();
    TextureResource* getAmbientOcclusionTexture();
    TextureResource* getSheenTexture();
    TextureResource* getClearcoatTexture();
    TextureResource* getTransmissionTexture();

    const glm::vec3& getDiffuseColor();
    const glm::vec3& getSpecularColor();
    const glm::vec3& getEmissiveColor();
    const glm::vec3& getAmbientColor();

    float getShininess();
    float getOpacity();
};

#endif
