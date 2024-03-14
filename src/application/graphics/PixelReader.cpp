#include "PixelReader.h"

#include <spdlog/spdlog.h>

#include "platform/Platform.h"

PixelReader::PixelReader()
    : mFrameBufferObject(-1)
    , mRenderTexture(-1)
    , mDepthBufferTexture(-1)
    , mDimensions(0.0f, 0.0f)
    , mNeedUpdate(true)
{

}

void PixelReader::setDimensions(const glm::vec2& pDimensions)
{
    mDimensions = pDimensions;
}

struct PixelInfo {
    unsigned int ObjectID = 0;
    unsigned int DrawID = 0;
    unsigned int PrimID = 0;

    void Print()
    {
        printf("Object %d draw %d prim %d\n", ObjectID, DrawID, PrimID);
    }
};

void PixelReader::read(glm::vec2 pCoordinate)
{
    if (mNeedUpdate)
    {
        update();
    }

    int tPreviousDrawFrameBufferObject = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER, &tPreviousDrawFrameBufferObject);

    // Enable writing
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBufferObject);

    // Render

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tPreviousDrawFrameBufferObject);

    int tPreviousReadFrameBufferObject = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER, &tPreviousReadFrameBufferObject);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFrameBufferObject);

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    PixelInfo Pixel;
    glReadPixels(pCoordinate.x, pCoordinate.y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &Pixel);

    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, tPreviousReadFrameBufferObject);







    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tPreviousReadFrameBufferObject);
}

void PixelReader::update()
{
    int tPreviousFrameBufferObject = -1;
    glGetIntegerv(GL_FRAMEBUFFER, &tPreviousFrameBufferObject);

    if (mFrameBufferObject == -1)
    {
        glGenFramebuffers(1, &mFrameBufferObject);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObject);

    if (mRenderTexture == -1)
    {
        glGenTextures(1, &mRenderTexture);
        glBindTexture(GL_TEXTURE_2D, mRenderTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, mDimensions.x, mDimensions.y, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mRenderTexture, 0);
    }

    if (mDepthBufferTexture == -1)
    {
        glGenTextures(1, &mDepthBufferTexture);
        glBindTexture(GL_TEXTURE_2D, mDepthBufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mDimensions.x, mDimensions.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthBufferTexture, 0);
    }

    GLenum tStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (tStatus != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::warn("{0}", tStatus);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, tPreviousFrameBufferObject);
}

void PixelReader::deleteBuffers()
{

}
