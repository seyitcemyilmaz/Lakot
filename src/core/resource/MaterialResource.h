#ifndef LAKOT_MATERIALRESOURCE_H
#define LAKOT_MATERIALRESOURCE_H

#include <glm/glm.hpp>

#include "TextureResource.h"

class MaterialResource
{
public:
    virtual ~MaterialResource();
    MaterialResource(
        const glm::vec3& pDiffuseColor,
        const glm::vec3& pSpecularColor,
        const glm::vec3& pEmissiveColor,
        const glm::vec3& pAmbientColor,
        float pShininess,
        float pOpacity);

    TextureResource* getDiffuseTexture() const;
    TextureResource* getSpecularTexture() const;
    TextureResource* getAmbientTexture() const;
    TextureResource* getEmissiveTexture() const;
    TextureResource* getHeightTexture() const;
    TextureResource* getNormalsTexture() const;
    TextureResource* getShininessTexture() const;
    TextureResource* getOpacityTexture() const;
    TextureResource* getDisplacementTexture() const;
    TextureResource* getLightmapTexture() const;
    TextureResource* getReflectionTexture() const;
    TextureResource* getBaseColorTexture() const;
    TextureResource* getNormalCameraTexture() const;
    TextureResource* getEmissionColorTexture() const;
    TextureResource* getMetalnessTexture() const;
    TextureResource* getDiffuseRoughnessTexture() const;
    TextureResource* getAmbientOcclusionTexture() const;
    TextureResource* getSheenTexture() const;
    TextureResource* getClearcoatTexture() const;
    TextureResource* getTransmissionTexture() const;

    const glm::vec3& getDiffuseColor() const;
    const glm::vec3& getSpecularColor() const;
    const glm::vec3& getEmissiveColor() const;
    const glm::vec3& getAmbientColor() const;

    float getShininess();
    float getOpacity();

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

private:
    glm::vec3 mDiffuseColor;
    glm::vec3 mSpecularColor;
    glm::vec3 mEmissiveColor;
    glm::vec3 mAmbientColor;

    float mShininess;
    float mOpacity;

    TextureResource* mDiffuseTexture;
    TextureResource* mSpecularTexture;
    TextureResource* mAmbientTexture;
    TextureResource* mEmissiveTexture;
    TextureResource* mHeightTexture;
    TextureResource* mNormalsTexture;
    TextureResource* mShininessTexture;
    TextureResource* mOpacityTexture;
    TextureResource* mDisplacementTexture;
    TextureResource* mLightmapTexture;
    TextureResource* mReflectionTexture;
    TextureResource* mBaseColorTexture;
    TextureResource* mNormalCameraTexture;
    TextureResource* mEmissionColorTexture;
    TextureResource* mMetalnessTexture;
    TextureResource* mDiffuseRoughnessTexture;
    TextureResource* mAmbientOcclusionTexture;
    TextureResource* mSheenTexture;
    TextureResource* mClearcoatTexture;
    TextureResource* mTransmissionTexture;
};

#endif
