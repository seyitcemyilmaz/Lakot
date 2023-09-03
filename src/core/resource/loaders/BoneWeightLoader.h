#ifndef LAKOT_BONELOADER_H
#define LAKOT_BONELOADER_H

#include <assimp/scene.h>

#include "resource/MeshResource.h"

class BoneWeightLoader {
private:
	aiBone* mBone;
	std::vector<Vertex>& mVertexList;
	unsigned int mBoneId;

	void setVertexBoneWeight(Vertex* pVertex, unsigned int pBoneId, float pWeight);

public:
	BoneWeightLoader(aiBone* pBone, std::vector<Vertex>& pVertexList, unsigned int pBoneId);

	void load();
};

#endif
