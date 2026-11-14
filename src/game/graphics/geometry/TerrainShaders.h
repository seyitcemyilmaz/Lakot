#ifndef LAKOT_TERRAIN_SHADERS_H
#define LAKOT_TERRAIN_SHADERS_H

#include <string>

namespace lakot
{

class TerrainShaders
{
public:
    static std::string vertexSource();
    static std::string fragmentSource(const std::string& pSharedFunctions);

private:
    TerrainShaders() = delete;
    ~TerrainShaders() = delete;
};

}

#endif
