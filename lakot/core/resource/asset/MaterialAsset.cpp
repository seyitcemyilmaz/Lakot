#include "MaterialAsset.h"

using namespace lakot;

MaterialAsset::~MaterialAsset()
{

}

MaterialAsset::MaterialAsset()
    : AAsset(AssetType::eMaterial)
    , mDiffuseColor(glm::vec3(0.0f, 0.0f, 0.0f))
    , mSpecularColor(glm::vec3(0.0f, 0.0f, 0.0f))
    , mEmissiveColor(glm::vec3(0.0f, 0.0f, 0.0f))
    , mAmbientColor(glm::vec3(0.0f, 0.0f, 0.0f))
    , mShininess(1.0f)
    , mOpacity(1.0f)
    , mDiffuseTexture(nullptr)
    , mSpecularTexture(nullptr)
    , mAmbientTexture(nullptr)
    , mEmissiveTexture(nullptr)
    , mHeightTexture(nullptr)
    , mNormalsTexture(nullptr)
    , mShininessTexture(nullptr)
    , mOpacityTexture(nullptr)
    , mDisplacementTexture(nullptr)
    , mLightmapTexture(nullptr)
    , mReflectionTexture(nullptr)
    , mBaseColorTexture(nullptr)
    , mNormalCameraTexture(nullptr)
    , mEmissionColorTexture(nullptr)
    , mMetalnessTexture(nullptr)
    , mDiffuseRoughnessTexture(nullptr)
    , mAmbientOcclusionTexture(nullptr)
    , mSheenTexture(nullptr)
    , mClearcoatTexture(nullptr)
    , mTransmissionTexture(nullptr)
{

}

void MaterialAsset::initialize()
{

}

void MaterialAsset::deinitialize()
{

}

const std::string& MaterialAsset::getName() const
{
    return mName;
}

const glm::vec3& MaterialAsset::getDiffuseColor() const
{
    return mDiffuseColor;
}

const glm::vec3& MaterialAsset::getSpecularColor() const
{
    return mSpecularColor;
}

const glm::vec3& MaterialAsset::getEmissiveColor() const
{
    return mEmissiveColor;
}

const glm::vec3& MaterialAsset::getAmbientColor() const
{
    return mAmbientColor;
}

float MaterialAsset::getShininess()
{
    return mShininess;
}

float MaterialAsset::getOpacity()
{
    return mOpacity;
}

TextureAsset* MaterialAsset::getDiffuseTexture() const
{
    return mDiffuseTexture;
}

TextureAsset* MaterialAsset::getSpecularTexture() const
{
    return mSpecularTexture;
}

TextureAsset* MaterialAsset::getAmbientTexture() const
{
    return mAmbientTexture;
}

TextureAsset* MaterialAsset::getEmissiveTexture() const
{
    return mEmissiveTexture;
}

TextureAsset* MaterialAsset::getHeightTexture() const
{
    return mHeightTexture;
}

TextureAsset* MaterialAsset::getNormalsTexture() const
{
    return mNormalsTexture;
}

TextureAsset* MaterialAsset::getShininessTexture() const
{
    return mShininessTexture;
}

TextureAsset* MaterialAsset::getOpacityTexture() const
{
    return mOpacityTexture;
}

TextureAsset* MaterialAsset::getDisplacementTexture() const
{
    return mDisplacementTexture;
}

TextureAsset* MaterialAsset::getLightmapTexture() const
{
    return mLightmapTexture;
}

TextureAsset* MaterialAsset::getReflectionTexture() const
{
    return mReflectionTexture;
}

TextureAsset* MaterialAsset::getBaseColorTexture() const
{
    return mBaseColorTexture;
}

TextureAsset* MaterialAsset::getNormalCameraTexture() const
{
    return mNormalCameraTexture;
}

TextureAsset* MaterialAsset::getEmissionColorTexture() const
{
    return mEmissionColorTexture;
}

TextureAsset* MaterialAsset::getMetalnessTexture() const
{
    return mMetalnessTexture;
}

TextureAsset* MaterialAsset::getDiffuseRoughnessTexture() const
{
    return mDiffuseRoughnessTexture;
}

TextureAsset* MaterialAsset::getAmbientOcclusionTexture() const
{
    return mAmbientOcclusionTexture;
}

TextureAsset* MaterialAsset::getSheenTexture() const
{
    return mSheenTexture;
}

TextureAsset* MaterialAsset::getClearcoatTexture() const
{
    return mClearcoatTexture;
}

TextureAsset* MaterialAsset::getTransmissionTexture() const
{
    return mTransmissionTexture;
}

void MaterialAsset::setName(const std::string& pName)
{
    mName = pName;
}

void MaterialAsset::setDiffuseColor(const glm::vec3& pDiffuseColor)
{
    mDiffuseColor = pDiffuseColor;
}

void MaterialAsset::setSpecularColor(const glm::vec3& pSpecularColor)
{
    mSpecularColor = pSpecularColor;
}

void MaterialAsset::setEmissiveColor(const glm::vec3& pEmissiveColor)
{
    mEmissiveColor = pEmissiveColor;
}

void MaterialAsset::setAmbientColor(const glm::vec3& pAmbientColor)
{
    mAmbientColor = pAmbientColor;
}

void MaterialAsset::setShininess(float pShininess)
{
    mShininess = pShininess;
}

void MaterialAsset::setOpacity(float pOpacity)
{
    mOpacity = pOpacity;
}

void MaterialAsset::setDiffuseTexture(TextureAsset* pTexture)
{
    mDiffuseTexture = pTexture;
}

void MaterialAsset::setSpecularTexture(TextureAsset* pTexture)
{
    mSpecularTexture = pTexture;
}

void MaterialAsset::setAmbientTexture(TextureAsset* pTexture)
{
    mAmbientTexture = pTexture;
}

void MaterialAsset::setEmissiveTexture(TextureAsset* pTexture)
{
    mEmissiveTexture = pTexture;
}

void MaterialAsset::setHeightTexture(TextureAsset* pTexture)
{
    mHeightTexture = pTexture;
}

void MaterialAsset::setNormalsTexture(TextureAsset* pTexture)
{
    mNormalsTexture = pTexture;
}

void MaterialAsset::setShininessTexture(TextureAsset* pTexture)
{
    mShininessTexture = pTexture;
}

void MaterialAsset::setOpacityTexture(TextureAsset* pTexture)
{
    mOpacityTexture = pTexture;
}

void MaterialAsset::setDisplacementTexture(TextureAsset* pTexture)
{
    mDisplacementTexture = pTexture;
}

void MaterialAsset::setLightmapTexture(TextureAsset* pTexture)
{
    mLightmapTexture = pTexture;
}

void MaterialAsset::setReflectionTexture(TextureAsset* pTexture)
{
    mReflectionTexture = pTexture;
}

void MaterialAsset::setBaseColorTexture(TextureAsset* pTexture)
{
    mBaseColorTexture = pTexture;
}

void MaterialAsset::setNormalCameraTexture(TextureAsset* pTexture)
{
    mNormalCameraTexture = pTexture;
}

void MaterialAsset::setEmissionColorTexture(TextureAsset* pTexture)
{
    mEmissionColorTexture = pTexture;
}

void MaterialAsset::setMetalnessTexture(TextureAsset* pTexture)
{
    mMetalnessTexture = pTexture;
}

void MaterialAsset::setDiffuseRoughnessTexture(TextureAsset* pTexture)
{
    mDiffuseRoughnessTexture = pTexture;
}

void MaterialAsset::setAmbientOcclusionTexture(TextureAsset* pTexture)
{
    mAmbientOcclusionTexture = pTexture;
}

void MaterialAsset::setSheenTexture(TextureAsset* pTexture)
{
    mSheenTexture = pTexture;
}

void MaterialAsset::setClearcoatTexture(TextureAsset* pTexture)
{
    mClearcoatTexture = pTexture;
}

void MaterialAsset::setTransmissionTexture(TextureAsset* pTexture)
{
    mTransmissionTexture = pTexture;
}
