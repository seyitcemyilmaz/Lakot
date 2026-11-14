#ifndef LAKOT_TERRAIN_MATERIAL_H
#define LAKOT_TERRAIN_MATERIAL_H

#include <memory>
#include <string>

#include "../model/Texture.h"
#include "../render/ShaderProgram.h"

namespace lakot
{

class TerrainMaterial
{
public:
    bool load(const std::string& pTextureDirectory, const std::string& pMapDirectory);

    bool isReady() const;

    void bind(ShaderProgram& pShader, float pWorldSize) const;

private:
    static constexpr unsigned int kFirstUnit = 1;

    std::shared_ptr<Texture> mSplat;
    std::shared_ptr<Texture> mGrass;
    std::shared_ptr<Texture> mDryGrass;
    std::shared_ptr<Texture> mDirt;
    std::shared_ptr<Texture> mRoad;
    std::shared_ptr<Texture> mRock;
    std::shared_ptr<Texture> mMacro;

    static std::shared_ptr<Texture> loadLayer(const std::string& pPath);
    static std::shared_ptr<Texture> loadSplat(const std::string& pPath);
};

}

#endif
