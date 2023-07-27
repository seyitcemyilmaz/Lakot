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

	std::vector<Vertex> mVertexList;
	std::vector<unsigned int> mIndexList;

	NodeResource* mConnectedNode;

	unsigned int mVAO;
	unsigned int mVBO;
	unsigned int mIBO;

	unsigned int mMaterialIndex;

	unsigned int mIndexCount;

	bool mHasBone;

protected:
	void setHasBone(bool pHasBone);
	void setConnectedNode(NodeResource* pConnectedNode);
	void createBuffers();
	friend class ModelLoader;

public:
	MeshResource(const std::string& pName, std::vector<Vertex> pVertexList, std::vector<unsigned int> pIndexList, unsigned int pMaterialIndex);

	std::string getName() const;
	unsigned int getMaterialIndex() const;

	void draw();
};

#endif
