#ifndef LAKOT_MESHASSET_H
#define LAKOT_MESHASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>
#include <lakot/abstract/graphics/model/AMesh.h>
#include <lakot/abstract/render/ARenderable.h>

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
#include "../../render/VertexArrayObject.h"
#endif

namespace lakot {

class MeshAsset
    : public AAsset
    , public AMesh
    , public ARenderable
{
public:
    virtual ~MeshAsset() override;
    MeshAsset(const std::string& pName, unsigned int pMaterialIndex, const BoundingBox& pBoundingBox);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const override;

    unsigned int getMaterialIndex() const;

    DataContainer* getVertexInformation() const;

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    const AVertexArrayObject& getVertexArrayObject() const override;
#endif

    bool getHasBones() const;

protected:
    friend class ModelLoader;

    void setVertexInformation(DataContainer* pVertexInformation);

    void setHasBones(bool pHasBones);

private:
    std::string mName;

    DataContainer* mVertexInformation;

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    VertexArrayObject mVertexArrayObject;
#endif

    unsigned int mMaterialIndex;

    bool mHasBones;
};

}

#endif
