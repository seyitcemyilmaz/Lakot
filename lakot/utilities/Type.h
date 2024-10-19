#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace lakot {

enum class AssetType
{
    eNone,
    eModel,
    eNode,
    eMesh,
    eMaterial,
    eTexture
};

enum class EntityType
{
    eNone,
    eModel,
    eNode,
    eMesh,
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
    eUnknown,
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

struct VertexInformation
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textureCoordinates;
    std::vector<glm::ivec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    std::vector<unsigned int> indices;
};

struct Image
{
    unsigned char* data;

    int width;
    int height;
    int channels;

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
};

template <class T>
struct Variable
{
    std::string name;
    T value;
};

}

#endif
