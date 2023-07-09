#ifndef LAKOT_MESHRESOURCE_H
#define LAKOT_MESHRESOURCE_H

#define LAKOT_VERTEX_MAX_BONE_COUNT				4

#include <vector>
#include <string>

#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoordinates;
	int boneIds[LAKOT_VERTEX_MAX_BONE_COUNT];
	float boneWeights[LAKOT_VERTEX_MAX_BONE_COUNT];
};

class NodeResource;

class MeshResource {
private:
	std::string mName;

	NodeResource* mConnectedNode;

	unsigned int mVAO;
	unsigned int mVBO;
	unsigned int mIBO;

	unsigned int mMaterialIndex;

	unsigned int mIndexCount;

	bool mHasBone;

protected:
	void setHasBone(bool pHasBone);
	void createBuffers(std::vector<Vertex>& tVertexList, std::vector<unsigned int>& tIndexList);
	friend class ModelLoader;

public:
	MeshResource(std::string pName, unsigned int pMaterialIndex, NodeResource* pConnectedNode);

	std::string getName();
	unsigned int getMaterialIndex();

	void draw();
};

#endif
