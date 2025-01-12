#ifndef LAKOT_TEXTUREASSET_H
#define LAKOT_TEXTUREASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>
#include <lakot/abstract/graphics/model/ATexture.h>

namespace lakot {

class TextureAsset
    : public AAsset
    , public ATexture
{
public:
    virtual ~TextureAsset() override;
    TextureAsset(const std::string& pPath, Image* pImage);

    void initialize() override;
    void deinitialize() override;

    const std::string& getPath() const override;

protected:
    friend class ModelLoader;

private:
    std::string mPath;

    Image* mImage;
};

}

#endif
