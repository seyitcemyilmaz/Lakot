#ifndef LAKOT_MESHRESOURCE_H
#define LAKOT_MESHRESOURCE_H

#define LAKOT_VERTEX_MAX_BONE_COUNT				4

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "helper/shader/IShader.h"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoordinates;
	int boneIds[LAKOT_VERTEX_MAX_BONE_COUNT];
	float boneWeights[LAKOT_VERTEX_MAX_BONE_COUNT];
};

class ModelResource;
class NodeResource;

class MeshResource {
private:
	std::string mName;

	ModelResource* mModelResource;

	std::vector<Vertex> mVertexList;
	std::vector<unsigned int> mIndexList;

	NodeResource* mConnectedNode = nullptr;

	unsigned int mVAO = 0;
	unsigned int mVBO = 0;
	unsigned int mIBO = 0;

	unsigned int mMaterialIndex = -1;

	unsigned int mIndexCount = 0;

	bool mHasBone = false;

protected:
	void setHasBone(bool pHasBone);
	void setConnectedNode(NodeResource* pConnectedNode);
	void createBuffers();
	friend class ModelLoader;

public:
	MeshResource(const std::string& pName, ModelResource* pModelResource, const std::vector<Vertex>& pVertexList, const std::vector<unsigned int>& pIndexList, unsigned int pMaterialIndex);

	std::string getName() const;
	unsigned int getMaterialIndex() const;

    void draw(IShader* pShader);
};

#endif
