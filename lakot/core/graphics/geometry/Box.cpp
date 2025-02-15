#include "Box.h"

#include <spdlog/spdlog.h>

using namespace lakot;

Box::~Box()
{

}

Box::Box()
    : ARenderable()
{
    mRenderableType = RenderableType::eBox;
}

void Box::initialize()
{
#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    mVertexInformation.set<glm::vec3>("positions",
    {
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f)
    });

    mVertexInformation.set<unsigned int>("indices",
    {
        0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
        6, 7, 7, 4, 0, 4, 3, 7, 1, 5, 2, 6
    });

    {
        VertexBufferObject* tBuffer =
            new VertexBufferObject("EBO",
                                   VertexBufferObjectBufferType::eElementArrayBuffer,
                                   VertexBufferObjectDataType::eUnsignedInt,
                                   VertexBufferObjectDrawType::eStaticDraw);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    {
        // Layout 0.
        VertexBufferObject* tBuffer =
            new VertexBufferObject("staticPosition",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec3,
                                   VertexBufferObjectDrawType::eStaticDraw);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    {
        // Layout 1.
        VertexBufferObject* tBuffer =
            new VertexBufferObject("dynamicPosition",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec3,
                                   VertexBufferObjectDrawType::eDynamicDraw);

        tBuffer->setIsInstanced(true);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer =
            new VertexBufferObject("dynamicSize",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec3,
                                   VertexBufferObjectDrawType::eDynamicDraw);

        tBuffer->setIsInstanced(true);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    mVertexArrayObject.initialize();

    {
        VertexBufferObject* tBuffer = mVertexArrayObject.getElementBufferObject();

        if (auto tIndices = mVertexInformation.get<unsigned int>("indices"))
        {
            tBuffer->setData(tIndices->get());
        }
        else
        {
            spdlog::error("Indices is not found.");
        }
    }

    {
        // Layout 0.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(0);

        if (auto tPositions = mVertexInformation.get<glm::vec3>("positions"))
        {
            tBuffer->setData(tPositions->get());
        }
        else
        {
            spdlog::error("Positions is not found.");
        }
    }

    {
        // Layout 1.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(1);

        std::vector<glm::vec3> tData =
        {
            glm::vec3(0.0f, 3.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f)
        };
        tBuffer->setData(tData);
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(2);

        std::vector<glm::vec3> tData =
        {
            glm::vec3(1.0f),
            glm::vec3(2.0f)
        };
        tBuffer->setData(tData);
    }
#endif
}

void Box::deinitialize()
{

}
#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
const AVertexArrayObject& Box::getVertexArrayObject() const
{
    return mVertexArrayObject;
}
#endif
