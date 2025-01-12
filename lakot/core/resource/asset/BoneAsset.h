#ifndef LAKOT_BONEASSET_H
#define LAKOT_BONEASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>
#include <lakot/abstract/graphics/model/ABone.h>

namespace lakot {

class BoneAsset
    : public AAsset
    , public ABone
{
public:
    virtual ~BoneAsset() override;
    BoneAsset(const std::string& pName, const glm::mat4& pTransformation);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const override;

private:
    std::string mName;
};

}

#endif
