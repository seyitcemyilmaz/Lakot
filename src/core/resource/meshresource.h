#ifndef LAKOT_MESHRESOURCE_H
#define LAKOT_MESHRESOURCE_H

#include "noderesource.h"
#include "textureresource.h"

#define LAKOT_VERTEX_MAX_BONE_COUNT				4

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoordinates;
	int boneIds[LAKOT_VERTEX_MAX_BONE_COUNT];
	float boneWeights[LAKOT_VERTEX_MAX_BONE_COUNT];
};

class MeshResource {
private:
	std::string mName;

	NodeResource* mConnectedNode;

	TextureResource* mTextureResource;

	unsigned int mVAO;
	unsigned int mVBO;
	unsigned int mIBO;

	bool mHasBone;
protected:
	void setHasBone(bool pHasBone);
	void createBuffers(std::vector<Vertex>* tVertexList, std::vector<unsigned int>* tIndexList);
	friend class ModelLoader;
public:
	MeshResource(std::string pName, NodeResource* pConnectedNode);

	std::string getName();
};

#endif
