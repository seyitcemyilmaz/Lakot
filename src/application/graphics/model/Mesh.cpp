#include "Mesh.h"

#include "Node.h"

Mesh::Mesh(MeshResource* pMeshResource, Node* pConnectedNode, unsigned int pMaterialIndex)
    : mMeshResource(pMeshResource)
    , mConnectedNode(pConnectedNode)
    , mMaterialIndex(pMaterialIndex)
    , mTransformationMatrix(glm::mat4(1.0f))
{

}

MeshResource* Mesh::getMeshResource()
{
    return mMeshResource;
}

const glm::mat4& Mesh::getTransformationMatrix() const {
    return mTransformationMatrix;
}

const std::string& Mesh::getName() const {
    return mMeshResource->getName();
}

unsigned int Mesh::getMaterialIndex() const {
    return mMaterialIndex;
}

bool Mesh::getHasBone() const {
    return mMeshResource->getHasBone();
}

void Mesh::calculateTransformationMatrix()
{
    glm::mat4 tTransformationMatrix = glm::mat4(1.0f);

    const INode* tParentNode = mConnectedNode;

    while (tParentNode)
    {
        tTransformationMatrix = tParentNode->getTransformationMatrix() * tTransformationMatrix;
        tParentNode = tParentNode->getParentNode();
    }

    mTransformationMatrix = tTransformationMatrix;
}
