#ifndef LAKOT_MESHASSET_H
#define LAKOT_MESHASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>

#include "../../geometry/BoundingBox.h"

namespace lakot {

class MeshAsset : public AAsset
{
public:
    virtual ~MeshAsset() override;
    MeshAsset(const std::string& pName, unsigned int pMaterialIndex);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const;

    unsigned int getMaterialIndex() const;

    BoundingBox* getBoundingBox() const;
    VertexInformation* getVertexInformation() const;

    bool getHasBones() const;

protected:
    friend class ModelLoader;

    void setBoundingBox(BoundingBox* pBoundingBox);
    void setVertexInformation(VertexInformation* pVertexInformation);

    void setHasBones(bool pHasBones);

private:
    std::string mName;

    BoundingBox* mBoundingBox;
    VertexInformation* mVertexInformation;

    unsigned int mMaterialIndex;

    bool mHasBones;
};

}

#endif
