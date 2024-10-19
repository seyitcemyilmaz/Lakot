#ifndef LAKOT_AASSET_H
#define LAKOT_AASSET_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AAsset : public Object
{
public:
    virtual ~AAsset() override;
    AAsset();

    const std::string& getUUID() const noexcept;
    void setUUID(const std::string& pUUID) noexcept;

    AssetType getType() const noexcept;

protected:
    std::string mUUID;

    AssetType mType;
};

}

#endif
