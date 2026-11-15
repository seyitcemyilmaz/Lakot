#ifndef LAKOT_WORLD_SHADERS_H
#define LAKOT_WORLD_SHADERS_H

namespace lakot
{

class ShaderManager;

class WorldShaders
{
public:
    static void ensurePrograms(ShaderManager& pShaderManager);

private:
    WorldShaders() = delete;
    ~WorldShaders() = delete;
};

}

#endif
