#include "MeshAsset.h"

#include <spdlog/spdlog.h>

using namespace lakot;

MeshAsset::~MeshAsset()
{

}

MeshAsset::MeshAsset(const std::string& pName, unsigned int pMaterialIndex, const BoundingBox& pBoundingBox)
    : AAsset(AssetType::eMesh)
    , AMesh(std::move(pBoundingBox))
    , ARenderable()
    , mName(pName)
    , mVertexInformation(nullptr)
    , mMaterialIndex(pMaterialIndex)
    , mHasBones(false)
{

}

void MeshAsset::initialize()
{
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
            new VertexBufferObject("position",
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
            new VertexBufferObject("normal",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec3,
                                   VertexBufferObjectDrawType::eStaticDraw);

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
            new VertexBufferObject("textureCoordinates",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec2,
                                   VertexBufferObjectDrawType::eStaticDraw);

        tBuffer->setIsInstanced(true);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    {
        // Layout 3.
        VertexBufferObject* tBuffer =
            new VertexBufferObject("boneIds",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eIVec4,
                                   VertexBufferObjectDrawType::eStaticDraw);

        tBuffer->setIsInstanced(true);

        bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

        if (!tIsAdded)
        {
            throw tBuffer->getName() + " is not added.";
        }
    }

    {
        // Layout 4.
        VertexBufferObject* tBuffer =
            new VertexBufferObject("boneWeights",
                                   VertexBufferObjectBufferType::eArrayBuffer,
                                   VertexBufferObjectDataType::eVec4,
                                   VertexBufferObjectDrawType::eStaticDraw);

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

        if (auto tIndices = mVertexInformation->get<unsigned int>("indices"))
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

        if (auto tPositions = mVertexInformation->get<glm::vec3>("positions"))
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

        if (auto tNormals = mVertexInformation->get<glm::vec3>("normals"))
        {
            tBuffer->setData(tNormals->get());
        }
        else
        {
            spdlog::error("Normals is not found.");
        }
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(2);

        if (auto tTextureCoordinates = mVertexInformation->get<glm::vec2>("textureCoordinates"))
        {
            tBuffer->setData(tTextureCoordinates->get());
        }
        else
        {
            spdlog::error("Texture Coordinates is not found.");
        }
    }

    {
        // Layout 3.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(3);

        if (auto tBoneIDs = mVertexInformation->get<glm::ivec4>("boneIDs"))
        {
            tBuffer->setData(tBoneIDs->get());
        }
        else
        {
            spdlog::error("Bone IDs is not found.");
        }
    }

    {
        // Layout 4.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(4);

        if (auto tBoneWeights = mVertexInformation->get<glm::vec4>("boneWeights"))
        {
            tBuffer->setData(tBoneWeights->get());
        }
        else
        {
            spdlog::error("Bone Weights is not found.");
        }
    }
#endif
}

void MeshAsset::deinitialize()
{
#if defined(LAKOT_RENDERER_OPENGL)
    // deinit
#endif
}

const std::string& MeshAsset::getName() const
{
    return mName;
}

DataContainer* MeshAsset::getVertexInformation() const
{
    return mVertexInformation;
}

#if defined(LAKOT_RENDERER_OPENGL)
const AVertexArrayObject& MeshAsset::getVertexArrayObject() const
{
    return mVertexArrayObject;
}
#endif

unsigned int MeshAsset::getMaterialIndex() const
{
    return mMaterialIndex;
}

bool MeshAsset::getHasBones() const
{
    return mHasBones;
}

void MeshAsset::setVertexInformation(DataContainer* pVertexInformation)
{
    mVertexInformation = pVertexInformation;
}

void MeshAsset::setHasBones(bool pHasBones)
{
    mHasBones = pHasBones;
}
