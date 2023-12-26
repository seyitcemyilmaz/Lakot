#ifndef LAKOT_MESHLOADER_H
#define LAKOT_MESHLOADER_H

#include <assimp/scene.h>

#include "resource/MeshResource.h"

class ModelResource;

class MeshLoader
{
private:
    aiMesh* mMeshObject;

    ModelResource* mModelResource;

    std::vector<Vertex> createVertexList();
    std::vector<unsigned int> createIndexList();

public:
    MeshLoader(aiMesh* pMeshObject, ModelResource* pModelResource);

    MeshResource* loadMesh();
};

#endif
