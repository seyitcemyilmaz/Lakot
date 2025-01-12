#include "MeshAsset.h"

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
#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
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
        tBuffer->setData(mVertexInformation->indices);
    }

    {
        // Layout 0.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(0);
        tBuffer->setData(mVertexInformation->positions);
    }

    {
        // Layout 1.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(1);
        tBuffer->setData(mVertexInformation->normals);
    }

    {
        // Layout 2.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(2);
        tBuffer->setData(mVertexInformation->textureCoordinates);
    }

    {
        // Layout 3.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(3);
        tBuffer->setData(mVertexInformation->boneIDs);
    }

    {
        // Layout 4.
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(4);
        tBuffer->setData(mVertexInformation->boneWeights);
    }
#endif
}

void MeshAsset::deinitialize()
{
#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    // deinit
#endif
}

const std::string& MeshAsset::getName() const
{
    return mName;
}

VertexInformation* MeshAsset::getVertexInformation() const
{
    return mVertexInformation;
}

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
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

void MeshAsset::setVertexInformation(VertexInformation* pVertexInformation)
{
    mVertexInformation = pVertexInformation;
}

void MeshAsset::setHasBones(bool pHasBones)
{
    mHasBones = pHasBones;
}
