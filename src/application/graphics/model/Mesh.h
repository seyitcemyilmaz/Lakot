#ifndef LAKOT_MESH_H
#define LAKOT_MESH_H

#include <vector>

#include "core/resource/MeshResource.h"

class Node;

class Mesh
{
private:
    MeshResource* mMeshResource;

    Node* mConnectedNode;

    unsigned int mMaterialIndex;

    glm::mat4 mTransformationMatrix;

public:
    Mesh(MeshResource* pMeshResource, Node* pConnectedNode, unsigned int pMaterialIndex);

    const std::string& getName() const;

    MeshResource* getMeshResource();

    const glm::mat4& getTransformationMatrix() const;

    unsigned int getMaterialIndex() const;
    bool getHasBone() const;

    void calculateTransformationMatrix();
};

#endif
