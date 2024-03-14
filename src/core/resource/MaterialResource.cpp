#include "MaterialResource.h"

MaterialResource::~MaterialResource()
{

}

MaterialResource::MaterialResource(const glm::vec3& pDiffuseColor,
                                   const glm::vec3& pSpecularColor,
                                   const glm::vec3& pEmissiveColor,
                                   const glm::vec3& pAmbientColor,
                                   float pShininess,
                                   float pOpacity)
    : mDiffuseColor(pDiffuseColor)
    , mSpecularColor(pSpecularColor)
    , mEmissiveColor(pEmissiveColor)
    , mAmbientColor(pAmbientColor)
    , mShininess(pShininess)
    , mOpacity(pOpacity)
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

TextureResource* MaterialResource::getDiffuseTexture() const
{
    return mDiffuseTexture;
}

TextureResource* MaterialResource::getSpecularTexture() const
{
    return mSpecularTexture;
}

TextureResource* MaterialResource::getAmbientTexture() const
{
    return mAmbientTexture;
}

TextureResource* MaterialResource::getEmissiveTexture() const
{
    return mEmissiveTexture;
}

TextureResource* MaterialResource::getHeightTexture() const
{
    return mHeightTexture;
}

TextureResource* MaterialResource::getNormalsTexture() const
{
    return mNormalsTexture;
}

TextureResource* MaterialResource::getShininessTexture() const
{
    return mShininessTexture;
}

TextureResource* MaterialResource::getOpacityTexture() const
{
    return mOpacityTexture;
}

TextureResource* MaterialResource::getDisplacementTexture() const
{
    return mDisplacementTexture;
}

TextureResource* MaterialResource::getLightmapTexture() const
{
    return mLightmapTexture;
}

TextureResource* MaterialResource::getReflectionTexture() const
{
    return mReflectionTexture;
}

TextureResource* MaterialResource::getBaseColorTexture() const
{
    return mBaseColorTexture;
}

TextureResource* MaterialResource::getNormalCameraTexture() const
{
    return mNormalCameraTexture;
}

TextureResource* MaterialResource::getEmissionColorTexture() const
{
    return mEmissionColorTexture;
}

TextureResource* MaterialResource::getMetalnessTexture() const
{
    return mMetalnessTexture;
}

TextureResource* MaterialResource::getDiffuseRoughnessTexture() const
{
    return mDiffuseRoughnessTexture;
}

TextureResource* MaterialResource::getAmbientOcclusionTexture() const
{
    return mAmbientOcclusionTexture;
}

TextureResource* MaterialResource::getSheenTexture() const
{
    return mSheenTexture;
}

TextureResource* MaterialResource::getClearcoatTexture() const
{
    return mClearcoatTexture;
}

TextureResource* MaterialResource::getTransmissionTexture() const
{
    return mTransmissionTexture;
}

const glm::vec3& MaterialResource::getDiffuseColor() const
{
    return mDiffuseColor;
}

const glm::vec3& MaterialResource::getSpecularColor() const
{
    return mSpecularColor;
}

const glm::vec3& MaterialResource::getEmissiveColor() const
{
    return mEmissiveColor;
}

const glm::vec3& MaterialResource::getAmbientColor() const
{
    return mAmbientColor;
}

float MaterialResource::getShininess()
{
    return mShininess;
}

float MaterialResource::getOpacity()
{
    return mOpacity;
}


void MaterialResource::setDiffuseTexture(TextureResource* pTextureResource)
{
    mDiffuseTexture = pTextureResource;
}

void MaterialResource::setSpecularTexture(TextureResource* pTextureResource)
{
    mSpecularTexture = pTextureResource;
}

void MaterialResource::setAmbientTexture(TextureResource* pTextureResource)
{
    mAmbientTexture = pTextureResource;
}

void MaterialResource::setEmissiveTexture(TextureResource* pTextureResource)
{
    mEmissiveTexture = pTextureResource;
}

void MaterialResource::setHeightTexture(TextureResource* pTextureResource)
{
    mHeightTexture = pTextureResource;
}

void MaterialResource::setNormalsTexture(TextureResource* pTextureResource)
{
    mNormalsTexture = pTextureResource;
}

void MaterialResource::setShininessTexture(TextureResource* pTextureResource)
{
    mShininessTexture = pTextureResource;
}

void MaterialResource::setOpacityTexture(TextureResource* pTextureResource)
{
    mOpacityTexture = pTextureResource;
}

void MaterialResource::setDisplacementTexture(TextureResource* pTextureResource)
{
    mDisplacementTexture = pTextureResource;
}

void MaterialResource::setLightmapTexture(TextureResource* pTextureResource)
{
    mLightmapTexture = pTextureResource;
}

void MaterialResource::setReflectionTexture(TextureResource* pTextureResource)
{
    mReflectionTexture = pTextureResource;
}

void MaterialResource::setBaseColorTexture(TextureResource* pTextureResource)
{
    mBaseColorTexture = pTextureResource;
}

void MaterialResource::setNormalCameraTexture(TextureResource* pTextureResource)
{
    mNormalCameraTexture = pTextureResource;
}

void MaterialResource::setEmissionColorTexture(TextureResource* pTextureResource)
{
    mEmissionColorTexture = pTextureResource;
}

void MaterialResource::setMetalnessTexture(TextureResource* pTextureResource)
{
    mMetalnessTexture = pTextureResource;
}

void MaterialResource::setDiffuseRoughnessTexture(TextureResource* pTextureResource)
{
    mDiffuseRoughnessTexture = pTextureResource;
}

void MaterialResource::setAmbientOcclusionTexture(TextureResource* pTextureResource)
{
    mAmbientOcclusionTexture = pTextureResource;
}

void MaterialResource::setSheenTexture(TextureResource* pTextureResource)
{
    mSheenTexture = pTextureResource;
}

void MaterialResource::setClearcoatTexture(TextureResource* pTextureResource)
{
    mClearcoatTexture = pTextureResource;
}

void MaterialResource::setTransmissionTexture(TextureResource* pTextureResource)
{
    mTransmissionTexture = pTextureResource;
}
