#include "OpenGLESRenderer.h"

#ifdef LAKOT_RENDERER_OPENGLES
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#endif

#include <spdlog/spdlog.h>

#include <lakot/abstract/render/AVertexArrayObject.h>

#include "ShaderSource.h"

using namespace lakot;

OpenGLESRenderer::~OpenGLESRenderer()
{

}

OpenGLESRenderer::OpenGLESRenderer()
    : ARenderer()
{

}

void OpenGLESRenderer::initialize()
{
    spdlog::info("OpenGLES Renderer is initializing.");

    mShaderPrograms["mesh"] =
        new OpenGLShaderProgram(
            "mesh",
            new OpenGLShader(MESH_VERTEX_SHADER, ShaderType::eVertex),
            new OpenGLShader(MESH_FRAGMENT_SHADER, ShaderType::eFragment));

    mShaderPrograms["box"] =
        new OpenGLShaderProgram(
            "box",
            new OpenGLShader(BOX_VERTEX_SHADER, ShaderType::eVertex),
            new OpenGLShader(BOX_FRAGMENT_SHADER, ShaderType::eFragment));

    for (const auto& tElement : mShaderPrograms)
    {
        tElement.second->initialize();
    }

    spdlog::info("OpenGLES Renderer is initialized.");
}

void OpenGLESRenderer::deinitialize()
{
    spdlog::info("OpenGLES Renderer is deinitializing.");

    for (const auto& tElement : mShaderPrograms)
    {
        tElement.second->deinitialize();
    }

    spdlog::info("OpenGLES Renderer is deinitialized.");
}

void OpenGLESRenderer::render(ARenderable* pRenderable)
{
    if (!pRenderable)
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    RenderableType tRenderableType = pRenderable->getRenderableType();

    if (tRenderableType == RenderableType::eMesh)
    {
        OpenGLShaderProgram* tShaderProgram = mShaderPrograms["mesh"];

        if (tShaderProgram)
        {
            // tShaderProgram->bind();
            // tShaderProgram->setMat4("projection", mProjectionMatrix);
            // tShaderProgram->setMat4("view", mViewMatrix);

            // const AVertexArrayObject& tVertexArrayObject = pRenderable->getVertexArrayObject();

            // tVertexArrayObject.bind();
            // unsigned int tIndiceCount = tVertexArrayObject.getIndiceCount();

            // // glDrawElements(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr);
            // glDrawElementsInstanced(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr, 2);

            spdlog::error("MODEL SHADER");
        }
    }
    else if (tRenderableType == RenderableType::eBox)
    {
        OpenGLShaderProgram* tShaderProgram = mShaderPrograms["box"];

        if (tShaderProgram)
        {
            tShaderProgram->bind();
            tShaderProgram->setMat4("projection", mProjectionMatrix);
            tShaderProgram->setMat4("view", mViewMatrix);

            const AVertexArrayObject& tVertexArrayObject = pRenderable->getVertexArrayObject();

            tVertexArrayObject.bind();
            unsigned int tIndiceCount = tVertexArrayObject.getIndiceCount();

            // glDrawElements(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr);
            glDrawElementsInstanced(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr, 2);
        }
    }

    GLenum tErrorCode;
    while ((tErrorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string tError;
        switch (tErrorCode)
        {
        case GL_INVALID_ENUM:                  tError = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 tError = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             tError = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                tError = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               tError = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 tError = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: tError = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        spdlog::error(tError);
    }
}
