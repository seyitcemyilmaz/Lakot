#include "ItemIconRenderer.h"

#include <algorithm>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "ItemAppearance.h"

#include "../gui/rml/RmlUiLayer.h"
#include "../graphics/render/ShaderProgram.h"
#include "../network/controller/InventoryController.h"

using namespace lakot;

namespace
{
    constexpr float kFill = 0.9f;
    const glm::vec3 kIconLightDirection = glm::normalize(glm::vec3(-0.25f, -0.55f, -0.8f));

    class SavedGlState
    {
    public:
        SavedGlState()
        {
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mFramebuffer);
            glGetIntegerv(GL_VIEWPORT, mViewport);
            glGetIntegerv(GL_CURRENT_PROGRAM, &mProgram);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mVertexArray);
            glGetIntegerv(GL_ACTIVE_TEXTURE, &mActiveTexture);
            glActiveTexture(GL_TEXTURE0);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &mTexture);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, mClearColor);
            mIsDepthTest = glIsEnabled(GL_DEPTH_TEST);
            mIsBlend = glIsEnabled(GL_BLEND);
            mIsScissor = glIsEnabled(GL_SCISSOR_TEST);
            mIsCullFace = glIsEnabled(GL_CULL_FACE);
            mIsStencil = glIsEnabled(GL_STENCIL_TEST);
        }

        ~SavedGlState()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(mFramebuffer));
            glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
            glUseProgram(static_cast<GLuint>(mProgram));
            glBindVertexArray(static_cast<GLuint>(mVertexArray));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(mTexture));
            glActiveTexture(static_cast<GLenum>(mActiveTexture));
            glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
            setEnabled(GL_DEPTH_TEST, mIsDepthTest);
            setEnabled(GL_BLEND, mIsBlend);
            setEnabled(GL_SCISSOR_TEST, mIsScissor);
            setEnabled(GL_CULL_FACE, mIsCullFace);
            setEnabled(GL_STENCIL_TEST, mIsStencil);
        }

    private:
        GLint mFramebuffer{0};
        GLint mViewport[4]{};
        GLint mProgram{0};
        GLint mVertexArray{0};
        GLint mActiveTexture{GL_TEXTURE0};
        GLint mTexture{0};
        GLfloat mClearColor[4]{};
        GLboolean mIsDepthTest{GL_FALSE};
        GLboolean mIsBlend{GL_FALSE};
        GLboolean mIsScissor{GL_FALSE};
        GLboolean mIsCullFace{GL_FALSE};
        GLboolean mIsStencil{GL_FALSE};

        static void setEnabled(GLenum pCapability, GLboolean pIsEnabled)
        {
            if (pIsEnabled)
            {
                glEnable(pCapability);
            }
            else
            {
                glDisable(pCapability);
            }
        }
    };
}

ItemIconRenderer::ItemIconRenderer(ItemAppearance& pAppearance, const InventoryController& pInventoryController,
                                   ShaderProgram& pShader, RmlUiLayer& pRmlUiLayer)
    : mAppearance(pAppearance)
    , mInventoryController(pInventoryController)
    , mShader(pShader)
    , mRmlUiLayer(pRmlUiLayer)
{

}

std::string ItemIconRenderer::getIconSource(const ItemVisual& pVisual)
{
    const ItemLook* tLook = mAppearance.find(pVisual);
    const ItemTemplate* tTemplate = mInventoryController.findTemplate(pVisual.templateId);

    if (!tLook || !tTemplate)
    {
        return std::string();
    }

    std::string tName = "item-" + std::to_string(pVisual.templateId) + "-" + std::to_string(pVisual.upgradeLevel);

    if (mRendered.insert(tName).second)
    {
        int tWidth = static_cast<int>(std::max(tTemplate->width, 1u)) * kPixelsPerCell;
        int tHeight = static_cast<int>(std::max(tTemplate->height, 1u)) * kPixelsPerCell;
        mRmlUiLayer.registerGeneratedImage(tName, tWidth, tHeight, render(*tLook, tWidth, tHeight));
    }

    return "lakot-generated/" + tName;
}

std::vector<uint8_t> ItemIconRenderer::render(const ItemLook& pLook, int pWidth, int pHeight) const
{
    SavedGlState tSaved;

    const int tWidth = pWidth * kSupersampling;
    const int tHeight = pHeight * kSupersampling;

    GLuint tFramebuffer = 0;
    GLuint tRenderbuffers[2] = {};

    glGenFramebuffers(1, &tFramebuffer);
    glGenRenderbuffers(2, tRenderbuffers);

    glBindRenderbuffer(GL_RENDERBUFFER, tRenderbuffers[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, tWidth, tHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, tRenderbuffers[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, tWidth, tHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, tFramebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, tRenderbuffers[0]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tRenderbuffers[1]);

    glViewport(0, 0, tWidth, tHeight);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const SkinnedModel& tModel = *pLook.model;
    const SkinnedModel::Bounds& tBounds = tModel.getBounds();
    glm::vec3 tSize = glm::max(tBounds.getSize(), glm::vec3(0.001f));
    glm::vec3 tCenter = tBounds.getCenter();

    float tUnitsPerPixel = std::max(tSize.x / static_cast<float>(pWidth), tSize.y / static_cast<float>(pHeight)) / kFill;
    float tHalfWidth = tUnitsPerPixel * static_cast<float>(pWidth) * 0.5f;
    float tHalfHeight = tUnitsPerPixel * static_cast<float>(pHeight) * 0.5f;

    glm::vec3 tEye = tCenter + glm::vec3(0.0f, 0.0f, tSize.z + 1.0f);
    glm::mat4 tView = glm::lookAt(tEye, tCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 tProjection = glm::ortho(-tHalfWidth, tHalfWidth, -tHalfHeight, tHalfHeight, 0.01f, tSize.z * 2.0f + 2.0f);

    const std::vector<glm::mat4>& tPalette = tModel.getBindPalette();

    mShader.bind();
    mShader.setMat4("uViewProjection", tProjection * tView);
    mShader.setVec3("uCameraPosition", tEye);
    mShader.setVec3("uFogColor", glm::vec3(0.0f));
    mShader.setFloat("uFogStart", 1.0e6f);
    mShader.setFloat("uFogEnd", 2.0e6f);
    mShader.setVec3("uLightDirection", kIconLightDirection);
    mShader.setInt("uTexture", 0);
    mShader.setMat4("uModel", glm::mat4(1.0f));
    mShader.setMat4Array("uBones", tPalette.data(), static_cast<unsigned int>(tPalette.size()));
    ItemAppearance::apply(mShader, pLook);

    tModel.draw(0);

    std::vector<uint8_t> tSamples(static_cast<size_t>(tWidth) * static_cast<size_t>(tHeight) * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, tWidth, tHeight, GL_RGBA, GL_UNSIGNED_BYTE, tSamples.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(2, tRenderbuffers);
    glDeleteFramebuffers(1, &tFramebuffer);

    // Box-filtered down and flipped to top-down rows; cleared pixels are zero, so the result stays premultiplied.
    std::vector<uint8_t> tPixels(static_cast<size_t>(pWidth) * static_cast<size_t>(pHeight) * 4);

    for (int tY = 0; tY < pHeight; ++tY)
    {
        for (int tX = 0; tX < pWidth; ++tX)
        {
            for (int tChannel = 0; tChannel < 4; ++tChannel)
            {
                int tSum = 0;

                for (int tSampleY = 0; tSampleY < kSupersampling; ++tSampleY)
                {
                    for (int tSampleX = 0; tSampleX < kSupersampling; ++tSampleX)
                    {
                        size_t tRow = static_cast<size_t>(tY * kSupersampling + tSampleY);
                        size_t tColumn = static_cast<size_t>(tX * kSupersampling + tSampleX);
                        tSum += tSamples[(tRow * static_cast<size_t>(tWidth) + tColumn) * 4 + static_cast<size_t>(tChannel)];
                    }
                }

                size_t tTarget = (static_cast<size_t>(pHeight - 1 - tY) * static_cast<size_t>(pWidth) + static_cast<size_t>(tX)) * 4
                               + static_cast<size_t>(tChannel);
                tPixels[tTarget] = static_cast<uint8_t>(tSum / (kSupersampling * kSupersampling));
            }
        }
    }

    return tPixels;
}
