#ifndef LAKOT_MAP_MODEL_BATCH_H
#define LAKOT_MAP_MODEL_BATCH_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace lakot
{

class Model;

class MapModelBatch
{
public:
    MapModelBatch(std::shared_ptr<Model> pModel, std::shared_ptr<Model> pLodModel);

    bool isValid() const { return mModel != nullptr; }
    void addInstance(const glm::vec3& pOffset, const glm::vec3& pParams);
    void update(const glm::vec3& pCamera);
    void draw(unsigned int pTextureUnit) const;

private:
    void rebuild(const glm::vec3& pCamera);

    std::shared_ptr<Model> mModel;
    std::shared_ptr<Model> mLodModel;
    std::vector<glm::vec3> mOffsets;
    std::vector<glm::vec3> mParams;
    glm::vec3 mLastCamera{0.0f};
    bool mIsDirty{true};
};

}

#endif
