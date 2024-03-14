#ifndef LAKOT_PIXELREADER_H
#define LAKOT_PIXELREADER_H

#include <glm/glm.hpp>

class PixelReader
{
public:
    PixelReader();

    void setDimensions(const glm::vec2& pDimensions);

    void read(glm::vec2 pCoordinate);

private:
    unsigned int mFrameBufferObject;
    unsigned int mRenderTexture;
    unsigned int mDepthBufferTexture;

    glm::vec2 mDimensions;

    bool mNeedUpdate;

    void update();
    void deleteBuffers();
};

#endif
