#ifndef LAKOT_BONEASSET_H
#define LAKOT_BONEASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>

namespace lakot {

class BoneAsset : public AAsset
{
public:
    virtual ~BoneAsset() override;
    BoneAsset(const std::string& pName);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const;

private:
    std::string mName;
};

}

#endif
