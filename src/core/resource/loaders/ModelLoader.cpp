#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "MaterialLoader.h"

ModelLoader::ModelLoader(std::string pModelPath) {
	mModelPath = pModelPath;

	mFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs | aiProcess_SplitByBoneCount;

	mModelResource = nullptr;
}

ModelResource* ModelLoader::loadModel() {
	Assimp::Importer tImporter;

	const aiScene* tScene = tImporter.ReadFile(mModelPath, mFlags);

	if (!tScene) {
		tImporter.FreeScene();
		delete tScene;
		return nullptr;
	}

	mModelResource = new ModelResource();

	processNode(tScene->mRootNode, tScene, nullptr);

	// TODO: Will be continued.

	extractMaterials(tScene);

	return mModelResource;
}

void ModelLoader::extractMaterials(const aiScene* pScene) {
	if (!pScene->HasMaterials()) {
		return;
	}

	MaterialLoader* tMaterialLoader = nullptr;

	unsigned int tMaterialCount = pScene->mNumMaterials;

	for (unsigned int i = 0; i < tMaterialCount; i++) {
		aiMaterial* tMaterial = pScene->mMaterials[i];
		tMaterialLoader = new MaterialLoader(tMaterial, mModelPath);

		MaterialResource* tMaterialResource = tMaterialLoader->loadMaterial();
		mModelResource->addMaterialResource(tMaterialResource);

		delete tMaterialLoader;
	}
}

void ModelLoader::processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource) {
	std::string tNodeName = pNode->mName.C_Str();
	glm::mat4 tTransformationMatrix = assimpMatrixToGlmMatrix(pNode->mTransformation);

	NodeResource* tNodeResource = new NodeResource(tNodeName, pParentNodeResource, tTransformationMatrix);

	if (pParentNodeResource) {
		pParentNodeResource->addChildNode(tNodeResource);
	}
	else {
		mModelResource->setRootNode(tNodeResource);
	}

	unsigned int tMeshCount = pNode->mNumMeshes;

	for (unsigned int i = 0; i < tMeshCount; i++) {
		processMesh(pScene->mMeshes[pNode->mMeshes[i]], pScene, tNodeResource);
	}

	unsigned int tChildNodeCount = pNode->mNumChildren;

	for (unsigned int i = 0; i < tChildNodeCount; i++) {
		processNode(pNode->mChildren[i], pScene, tNodeResource);
	}
}

void ModelLoader::processMesh(aiMesh* pMesh, const aiScene* pScene, NodeResource* pConnectedNodeResource) {
	std::string tMeshName = pMesh->mName.C_Str();
	unsigned int tMaterialIndex = pMesh->mMaterialIndex;

	std::vector<Vertex> tVertexList = createVertexList(pMesh);
	std::vector<unsigned int> tIndexList = createIndexList(pMesh);

	MeshResource* tMeshResource = new MeshResource(tMeshName, tMaterialIndex, pConnectedNodeResource);

	if (pMesh->HasBones()) {
		extractBones(pMesh, tVertexList);
		normalizeVertexBoneWeights(tVertexList);
		tMeshResource->setHasBone(true);
	}

	tMeshResource->createBuffers(tVertexList, tIndexList);

	pConnectedNodeResource->addChildMesh(tMeshResource);
	mModelResource->addMeshResource(tMeshResource);
}

void ModelLoader::extractBones(aiMesh* pMesh, std::vector<Vertex>& pVertexList) {
	aiBone** tBones = pMesh->mBones;
	unsigned int tBoneCount = pMesh->mNumBones;

	for (unsigned int i = 0; i < tBoneCount; i++) {
		aiBone* tBone = tBones[i];
		std::string tBoneName = tBone->mName.C_Str();

		if (!mModelResource->mBoneMap.contains(tBoneName)) {
			BoneResource* tBoneResource = new BoneResource(tBoneName, assimpMatrixToGlmMatrix(tBone->mOffsetMatrix));
			mModelResource->mBoneMap[tBoneName] = tBoneResource;
			mModelResource->addBoneResource(tBoneResource);
		}

		extractVertexBoneWeights(tBone, pVertexList);
	}
}

void ModelLoader::extractVertexBoneWeights(aiBone* pBone, std::vector<Vertex>& pVertexList) {
	int tBoneId = mModelResource->getBoneId(pBone->mName.C_Str());

	unsigned int tWeightCount = pBone->mNumWeights;
	aiVertexWeight* tWeights = pBone->mWeights;

	for (unsigned int i = 0; i < tWeightCount; i++) {
		float tVertexWeight = tWeights[i].mWeight;
		unsigned int tVertexIndex = tWeights[i].mVertexId;
		Vertex* tVertex = &pVertexList[tVertexIndex];

		setVertexBoneWeight(tVertex, tBoneId, tVertexWeight);
	}
}

void ModelLoader::normalizeVertexBoneWeights(std::vector<Vertex>& pVertexList) {
	size_t tVertexCount = pVertexList.size();

	for (size_t i = 0; i < tVertexCount; i++) {
		Vertex* tVertex = &pVertexList[i];

		if (tVertex->boneIds[0] != -1) {
			unsigned int tVertexBoneCount = 0;
			float tTotalWeight = 0.0f;

			for (unsigned int j = 0; j < LAKOT_VERTEX_MAX_BONE_COUNT; j++) {
				if (tVertex->boneIds[j] == -1) {
					break;
				}
				tTotalWeight += tVertex->boneWeights[j];
				tVertexBoneCount++;
			}

			if (std::abs(1.0f - tTotalWeight) > 0.01) {
				for (unsigned int j = 0; j < tVertexBoneCount; j++) {
					tVertex->boneWeights[j] /= tTotalWeight;
				}
			}
		}
	}
}

void ModelLoader::setVertexBoneWeight(Vertex* pVertex, int pBoneId, float pWeight) {
	if (pWeight == 0.0f) {
		return;
	}

	for (int i = 0; i < LAKOT_VERTEX_MAX_BONE_COUNT; i++) {
		if (pVertex->boneIds[i] == pBoneId) {
			break;
		}

		if (pVertex->boneIds[i] == -1) {
			pVertex->boneIds[i] = pBoneId;
			pVertex->boneWeights[i] = pWeight;
			break;
		}
	}
}

std::vector<Vertex> ModelLoader::createVertexList(aiMesh* pMesh) {
	std::vector<Vertex> tVertexList;
	unsigned int tVertexCount = pMesh->mNumVertices;

	for (unsigned int i = 0; i < tVertexCount; i++) {
		Vertex tVertex {
			assimp3DVectorToGlmVector(pMesh->mVertices[i]),
					assimp3DVectorToGlmVector(pMesh->mNormals[i])
		};

		if (pMesh->mTextureCoords[0]) {
			tVertex.textureCoordinates = glm::vec2(pMesh->mTextureCoords[0]->x, pMesh->mTextureCoords[0]->y);
		}
		else {
			tVertex.textureCoordinates = glm::vec2(0.0f);
		}

		for (unsigned int j = 0; j < LAKOT_VERTEX_MAX_BONE_COUNT; j++) {
			tVertex.boneIds[j] = -1;
			tVertex.boneWeights[j] = 0.0f;
		}

		tVertexList.push_back(tVertex);
	}

	return tVertexList;
}

std::vector<unsigned int> ModelLoader::createIndexList(aiMesh* pMesh) {
	std::vector<unsigned int> tIndexList;
	unsigned int tPolygonCount = pMesh->mNumFaces;

	for (unsigned int i = 0; i < tPolygonCount; i++) {
		if (pMesh->mFaces[i].mNumIndices != 3) {
			throw "A polygon has not 3 indices";
		}

		tIndexList.push_back(pMesh->mFaces[i].mIndices[0]);
		tIndexList.push_back(pMesh->mFaces[i].mIndices[1]);
		tIndexList.push_back(pMesh->mFaces[i].mIndices[2]);
	}

	return tIndexList;
}

glm::mat4 ModelLoader::assimpMatrixToGlmMatrix(aiMatrix4x4& pMatrix) {
	return glm::mat4 {
		pMatrix.a1, pMatrix.b1, pMatrix.c1, pMatrix.d1,
		pMatrix.a2, pMatrix.b2, pMatrix.c2, pMatrix.d2,
		pMatrix.a3, pMatrix.b3, pMatrix.c3, pMatrix.d3,
		pMatrix.a4, pMatrix.b4, pMatrix.c4, pMatrix.d4
	};
}

glm::vec3 ModelLoader::assimp3DVectorToGlmVector(aiVector3D& pVector) {
	return glm::vec3 {
		pVector.x, pVector.y, pVector.z
	};
}
