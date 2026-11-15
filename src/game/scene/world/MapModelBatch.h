#ifndef LAKOT_MAP_MODEL_BATCH_H
#define LAKOT_MAP_MODEL_BATCH_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace lakot
{

class Model;
struct Frustum;

class MapModelBatch
{
public:
    MapModelBatch(std::shared_ptr<Model> pModel, std::shared_ptr<Model> pLodModel);

    bool isValid() const { return mModel != nullptr; }
    void setDrawDistance(float pDistance) { mDrawDistance = pDistance; }
    void addInstance(const glm::vec3& pOffset, const glm::vec3& pParams);
    void update(const glm::vec3& pCamera, float pViewDistance, const Frustum& pFrustum);
    void draw(unsigned int pTextureUnit) const;

private:
    struct Cell
    {
        std::vector<uint32_t> indices;
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
        float maxExtent{0.0f};
    };

    void buildCells();
    float getModelRadius() const;
    float getCullDistance(float pViewDistance) const;
    void collectVisibleCells(const glm::vec3& pCamera, float pCullDistance, const Frustum& pFrustum, std::vector<uint32_t>& pOut) const;
    void rebuild(const glm::vec3& pCamera, float pViewDistance);

    std::shared_ptr<Model> mModel;
    std::shared_ptr<Model> mLodModel;
    std::vector<glm::vec3> mOffsets;
    std::vector<glm::vec3> mParams;
    std::vector<Cell> mCells;
    std::vector<uint32_t> mVisibleCells;
    std::vector<uint32_t> mScratchCells;
    glm::vec3 mLastCamera{0.0f};
    float mDrawDistance{0.0f};
    float mLastViewDistance{0.0f};
    bool mIsDirty{true};
    bool mAreCellsDirty{true};
};

}

#endif
