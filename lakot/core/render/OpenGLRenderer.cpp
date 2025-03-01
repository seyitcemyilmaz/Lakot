#include "OpenGLRenderer.h"

#if defined(LAKOT_RENDERER_OPENGL)
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
#include <GL/glew.h>
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#else
#error "Undefined OpenGL Type."
#endif
#endif

#include <spdlog/spdlog.h>

#include <lakot/abstract/render/AVertexArrayObject.h>

#include "../graphics/geometry/Box.h"

#include "ShaderSource.h"

using namespace lakot;

OpenGLRenderer::~OpenGLRenderer()
{

}

OpenGLRenderer::OpenGLRenderer()
    : ARenderer()
{

}

void OpenGLRenderer::initialize()
{
    spdlog::info("OpenGL Renderer is initializing.");

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

    spdlog::info("OpenGL Renderer is initialized.");
}

void OpenGLRenderer::deinitialize()
{
    spdlog::info("OpenGL Renderer is deinitializing.");

    for (const auto& tElement : mShaderPrograms)
    {
        tElement.second->deinitialize();
    }

    spdlog::info("OpenGL Renderer is deinitialized.");
}

void OpenGLRenderer::render(ARenderable* pRenderable)
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
            tShaderProgram->bind();
            tShaderProgram->setMat4("projection", mProjectionMatrix);
            tShaderProgram->setMat4("view", mViewMatrix);

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

        Box* tBox = dynamic_cast<Box*>(pRenderable);

        if (tBox)
        {
            if (tShaderProgram)
            {
                tShaderProgram->bind();
                tShaderProgram->setMat4("projection", mProjectionMatrix);
                tShaderProgram->setMat4("view", mViewMatrix);

                const AVertexArrayObject& tVertexArrayObject = pRenderable->getVertexArrayObject();

                tVertexArrayObject.bind();
                unsigned int tIndiceCount = tVertexArrayObject.getIndiceCount();

                //tbox

                // glDrawElements(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr);
                glDrawElementsInstanced(GL_LINES, tIndiceCount, GL_UNSIGNED_INT, nullptr, 2);
            }
        }
        else
        {
            spdlog::error("Box object is not found.");
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
