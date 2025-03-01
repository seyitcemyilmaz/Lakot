#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

#include <string>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

namespace lakot {

enum class AssetType
{
    eUndefined,
    eModel,
    eNode,
    eMesh,
    eBone,
    eMaterial,
    eTexture
};

enum class EntityType
{
    eUndefined,
    eModel,
    eNode,
    eMesh,
    eBone,
    eMaterial,
    eTexture
};

enum class ButtonType
{
    eUndefined,
    eLeft,
    eRight,
    eMiddle
};

enum class RenderableType
{
    eUndefined,
    eMesh,
    eBox
};

enum class CameraType
{
    eUndefined,
    eFPS
};

enum class GraphicsAPIType
{
    eUndefined,
    eOpenGL
};

enum class OpenGLType
{
    eUndefined,
    eCore,
    eES
};

enum class ShaderType
{
    eUndefined,
    eVertex,
    eFragment
};

struct ShaderUniform
{
    ShaderUniform(const std::string& pName)
        : name(pName)
        , location(-1)
    {

    }

    std::string name;
    int location;
};

struct Process
{
    Process(const std::function<void()>& pExecuteFunction, const std::function<void()>& pProcessCompletedCallback = nullptr)
    {
        executeFunction = pExecuteFunction;
        processCompletedCallback = pProcessCompletedCallback;
    }

    std::function<void()> executeFunction;
    std::function<void()> processCompletedCallback;
};

struct Image
{
    ~Image()
    {
        if (data)
        {
            delete [] data;
            data = nullptr;
        }
    }

    Image()
        : data(nullptr)
        , width(-1)
        , height(-1)
        , channels(-1)
    {

    }

    unsigned char* data;

    int width;
    int height;
    int channels;
};

template <class T>
struct Variable
{
    std::string name;
    T value;
};

}

#endif
