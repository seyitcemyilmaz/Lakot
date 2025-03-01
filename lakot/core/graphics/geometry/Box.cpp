#include "Box.h"

#include <spdlog/spdlog.h>

using namespace lakot;

Box::~Box()
{

}

Box::Box()
    : ARenderable()
    , mIsInitialized(false)
    , mIsNeedUpdate(true)
{
    mRenderableType = RenderableType::eBox;
}

void Box::initialize()
{
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

#if defined(LAKOT_RENDERER_OPENGL)
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
        const std::vector<glm::vec3>& tDynamicPosition = mVertexInformation.getBuffer<glm::vec3>("dynamicPosition");
        tBuffer->setData(tDynamicPosition);
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(2);
        const std::vector<glm::vec3>& tDynamicSize = mVertexInformation.getBuffer<glm::vec3>("dynamicSize");
        tBuffer->setData(tDynamicSize);
    }
#elif
#error "Undefined Graphics API";
#endif

    mIsNeedUpdate = false;
    mIsInitialized = true;
}

void Box::deinitialize()
{

}

void Box::addBox(const glm::vec3& pPosition, const glm::vec3& pSize)
{
    mVertexInformation.append<glm::vec3>("dynamicPosition", pPosition);
    mVertexInformation.append<glm::vec3>("dynamicSize", pSize);

    mIsNeedUpdate = true;

    if (!mIsInitialized)
    {
        return;
    }

#if defined(LAKOT_RENDERER_OPENGL)
    {
        // Layout 1.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(1);
        const std::vector<glm::vec3>& tDynamicPosition = mVertexInformation.getBuffer<glm::vec3>("dynamicPosition");
        tBuffer->setData(tDynamicPosition);
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(2);
        const std::vector<glm::vec3>& tDynamicSize = mVertexInformation.getBuffer<glm::vec3>("dynamicSize");
        tBuffer->setData(tDynamicSize);
    }
#elif
#error "Undefined Graphics API";
#endif

    mIsNeedUpdate = false;
}

#if defined(LAKOT_RENDERER_OPENGL)
const AVertexArrayObject& Box::getVertexArrayObject() const
{
    return mVertexArrayObject;
}
#endif
