#ifndef LAKOT_VERTEXBUFFEROBJECT_H
#define LAKOT_VERTEXBUFFEROBJECT_H

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include <lakot/utilities/Object.h>

namespace lakot {

enum class VertexBufferObjectBufferType
{
    eElementArrayBuffer,
    eArrayBuffer
};

enum class VertexBufferObjectDrawType
{
    eStaticDraw,
    eDynamicDraw,
    eStreamDraw
};

enum class VertexBufferObjectDataType
{
    eVec2,
    eVec3,
    eVec4,
    eIVec2,
    eIVec3,
    eIVec4,
    eUnsignedInt
};

class VertexBufferObject : public Object
{
public:
    virtual ~VertexBufferObject() override;
    VertexBufferObject(const VertexBufferObject &) = default;
    VertexBufferObject(VertexBufferObject &&) = delete;
    VertexBufferObject &operator=(const VertexBufferObject &) = default;
    VertexBufferObject &operator=(VertexBufferObject &&) = delete;
    VertexBufferObject(const std::string &pName,
                       VertexBufferObjectBufferType pBufferType,
                       VertexBufferObjectDataType pDataType,
                       VertexBufferObjectDrawType pDrawType);

    void initialize() override;
    void deinitialize() override;

    void bind();

    const std::string& getName() const;

    VertexBufferObjectBufferType getBufferType() const;

    VertexBufferObjectDrawType getDrawType() const;
    VertexBufferObjectDataType getDataType() const;

    unsigned int getDataCount() const;

    void setLayoutLocation(unsigned int pLayoutLocation);

    bool getIsInstanced() const;
    void setIsInstanced(bool pIsInstanced);

    void setData(const std::vector<unsigned int>& pData);

    void setData(const std::vector<glm::vec2>& pData);
    void setData(const std::vector<glm::vec3>& pData);
    void setData(const std::vector<glm::vec4>& pData);

    void setData(const std::vector<glm::ivec2>& pData);
    void setData(const std::vector<glm::ivec3>& pData);
    void setData(const std::vector<glm::ivec4>& pData);

private:
    std::string mName;

    unsigned int mId;

    unsigned int mLayoutLocation;

    unsigned int mDataCount;

    VertexBufferObjectBufferType mBufferType;
    VertexBufferObjectDrawType mDrawType;
    VertexBufferObjectDataType mDataType;

    bool mIsInstanced;

    unsigned int getTypeInternal() const;
    unsigned int getDrawTypeInternal() const;
};

}

#endif
