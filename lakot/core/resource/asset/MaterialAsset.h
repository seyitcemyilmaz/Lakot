#ifndef LAKOT_MATERIALASSET_H
#define LAKOT_MATERIALASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>

namespace lakot {

class TextureAsset;

class MaterialAsset : public AAsset
{
public:
    virtual ~MaterialAsset();
    MaterialAsset();

    void initialize();
    void deinitialize();

    const std::string& getName() const;

    const glm::vec3& getDiffuseColor() const;
    const glm::vec3& getSpecularColor() const;
    const glm::vec3& getEmissiveColor() const;
    const glm::vec3& getAmbientColor() const;

    float getShininess();
    float getOpacity();

    TextureAsset* getDiffuseTexture() const;
    TextureAsset* getSpecularTexture() const;
    TextureAsset* getAmbientTexture() const;
    TextureAsset* getEmissiveTexture() const;
    TextureAsset* getHeightTexture() const;
    TextureAsset* getNormalsTexture() const;
    TextureAsset* getShininessTexture() const;
    TextureAsset* getOpacityTexture() const;
    TextureAsset* getDisplacementTexture() const;
    TextureAsset* getLightmapTexture() const;
    TextureAsset* getReflectionTexture() const;
    TextureAsset* getBaseColorTexture() const;
    TextureAsset* getNormalCameraTexture() const;
    TextureAsset* getEmissionColorTexture() const;
    TextureAsset* getMetalnessTexture() const;
    TextureAsset* getDiffuseRoughnessTexture() const;
    TextureAsset* getAmbientOcclusionTexture() const;
    TextureAsset* getSheenTexture() const;
    TextureAsset* getClearcoatTexture() const;
    TextureAsset* getTransmissionTexture() const;

protected:
    friend class ModelLoader;

    void setName(const std::string& pName);

    void setDiffuseColor(const glm::vec3& pDiffuseColor);
    void setSpecularColor(const glm::vec3& pSpecularColor);
    void setEmissiveColor(const glm::vec3& pEmissiveColor);
    void setAmbientColor(const glm::vec3& pAmbientColor);

    void setShininess(float pShininess);
    void setOpacity(float pOpacity);

    void setDiffuseTexture(TextureAsset* pTexture);
    void setSpecularTexture(TextureAsset* pTexture);
    void setAmbientTexture(TextureAsset* pTexture);
    void setEmissiveTexture(TextureAsset* pTexture);
    void setHeightTexture(TextureAsset* pTexture);
    void setNormalsTexture(TextureAsset* pTexture);
    void setShininessTexture(TextureAsset* pTexture);
    void setOpacityTexture(TextureAsset* pTexture);
    void setDisplacementTexture(TextureAsset* pTexture);
    void setLightmapTexture(TextureAsset* pTexture);
    void setReflectionTexture(TextureAsset* pTexture);
    void setBaseColorTexture(TextureAsset* pTexture);
    void setNormalCameraTexture(TextureAsset* pTexture);
    void setEmissionColorTexture(TextureAsset* pTexture);
    void setMetalnessTexture(TextureAsset* pTexture);
    void setDiffuseRoughnessTexture(TextureAsset* pTexture);
    void setAmbientOcclusionTexture(TextureAsset* pTexture);
    void setSheenTexture(TextureAsset* pTexture);
    void setClearcoatTexture(TextureAsset* pTexture);
    void setTransmissionTexture(TextureAsset* pTexture);

private:
    std::string mName;

    glm::vec3 mDiffuseColor;
    glm::vec3 mSpecularColor;
    glm::vec3 mEmissiveColor;
    glm::vec3 mAmbientColor;

    float mShininess;
    float mOpacity;

    TextureAsset* mDiffuseTexture;
    TextureAsset* mSpecularTexture;
    TextureAsset* mAmbientTexture;
    TextureAsset* mEmissiveTexture;
    TextureAsset* mHeightTexture;
    TextureAsset* mNormalsTexture;
    TextureAsset* mShininessTexture;
    TextureAsset* mOpacityTexture;
    TextureAsset* mDisplacementTexture;
    TextureAsset* mLightmapTexture;
    TextureAsset* mReflectionTexture;
    TextureAsset* mBaseColorTexture;
    TextureAsset* mNormalCameraTexture;
    TextureAsset* mEmissionColorTexture;
    TextureAsset* mMetalnessTexture;
    TextureAsset* mDiffuseRoughnessTexture;
    TextureAsset* mAmbientOcclusionTexture;
    TextureAsset* mSheenTexture;
    TextureAsset* mClearcoatTexture;
    TextureAsset* mTransmissionTexture;
};

}

#endif
