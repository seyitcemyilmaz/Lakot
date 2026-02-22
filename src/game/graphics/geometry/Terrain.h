#ifndef LAKOT_TERRAIN_H
#define LAKOT_TERRAIN_H

#include <glm/glm.hpp>

#include "../render/Renderable.h"

namespace lakot
{

class Terrain final : public Renderable
{
public:
    ~Terrain() override;
    Terrain(int pWidth, int pDepth, float pGridSpacing);

    void initialize() override;
    void deinitialize() override;

    int getIndexCount() const;

private:
    int mWidth;
    int mDepth;

    float mGridSpacing;
    int mIndexCount;

    void generateMesh();
};

}

#endif
