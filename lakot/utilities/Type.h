#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

#include <string>
#include <vector>

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
    eOpenGL,
    eOpenGLES
};

enum class ShaderType
{
    eUndefined,
    eVertex,
    eFragment
};

struct VertexInformation
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textureCoordinates;
    std::vector<glm::ivec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    std::vector<unsigned int> indices;
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
