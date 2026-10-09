#ifndef LAKOT_MODEL_H
#define LAKOT_MODEL_H

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Texture.h"

#include "../render/Renderable.h"

namespace lakot
{

class AssetManager;

// A textured model (GLB, loaded through Assimp), drawn instanced: one draw
// call for every placement of it on the map. All meshes of the file are
// merged into one buffer; the first material's base colour texture is used
// for all of them - which is how kit-style asset packs (one shared colour
// atlas per kit) are built.
class Model final : public Renderable
{
public:
    // nullptr on failure, with the reason in pError.
    static std::unique_ptr<Model> load(const std::string& pPath, AssetManager& pAssets, std::string& pError);

    ~Model() override;

    void initialize() override;
    void deinitialize() override;

    // One placement per entry: where the model's origin goes, and its yaw
    // (radians about +y), uniform scale and extra stretch along its own x.
    void setInstances(const std::vector<glm::vec3>& pOffsets, const std::vector<glm::vec3>& pYawScaleStretches);

    // Binds the texture to pTextureUnit and draws every instance. The
    // shader must already be bound.
    void draw(unsigned int pTextureUnit) const;

private:
    Model();

    unsigned int mIndexCount{0};
    unsigned int mInstanceCount{0};

    std::shared_ptr<Texture> mTexture;
};

}

#endif
