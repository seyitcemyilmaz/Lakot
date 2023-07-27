#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "helper/AssimpToGLMHelper.h"

#include "BoneWeightLoader.h"
#include "MaterialLoader.h"
#include "MeshLoader.h"

ModelLoader::ModelLoader(const std::string& pModelPath) : mModelPath(pModelPath) {
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
	glm::mat4 tTransformationMatrix = AssimpToGLMHelper::toMat4(pNode->mTransformation);

	NodeResource* tNodeResource = new NodeResource(tNodeName, pParentNodeResource, tTransformationMatrix);

	if (pParentNodeResource) {
		pParentNodeResource->addChildNode(tNodeResource);
	}
	else {
		mModelResource->setRootNode(tNodeResource);
	}

	unsigned int tMeshCount = pNode->mNumMeshes;

	for (unsigned int i = 0; i < tMeshCount; i++) {
		processMesh(pScene->mMeshes[pNode->mMeshes[i]], tNodeResource);
	}

	unsigned int tChildNodeCount = pNode->mNumChildren;

	for (unsigned int i = 0; i < tChildNodeCount; i++) {
		processNode(pNode->mChildren[i], pScene, tNodeResource);
	}
}

void ModelLoader::processMesh(aiMesh* pMesh, NodeResource* pConnectedNodeResource) {
	MeshLoader tMeshLoader(pMesh);
	MeshResource* tMeshResource = tMeshLoader.loadMesh();

	if (pMesh->HasBones()) {
		extractBones(pMesh, tMeshResource);
		tMeshResource->setHasBone(true);
	}

	tMeshResource->createBuffers();

	tMeshResource->setConnectedNode(pConnectedNodeResource);
	pConnectedNodeResource->addChildMesh(tMeshResource);

	mModelResource->addMeshResource(tMeshResource);
}

void ModelLoader::extractBones(aiMesh* pMesh, MeshResource* pMeshResource) {
	aiBone** tBones = pMesh->mBones;
	unsigned int tBoneCount = pMesh->mNumBones;

	for (unsigned int i = 0; i < tBoneCount; i++) {
		aiBone* tBone = tBones[i];
		std::string tBoneName = tBone->mName.C_Str();

		if (!mModelResource->mBoneMap.contains(tBoneName)) {
			BoneResource* tBoneResource = new BoneResource(tBoneName, AssimpToGLMHelper::toMat4(tBone->mOffsetMatrix));
			mModelResource->mBoneMap[tBoneName] = tBoneResource;
			mModelResource->addBoneResource(tBoneResource);
		}

		unsigned int tBoneId = mModelResource->getBoneId(tBoneName);

		BoneWeightLoader tBoneWeightLoader(tBone, pMeshResource->mVertexList, tBoneId);
		tBoneWeightLoader.load();
	}

	normalizeBoneWeights(pMeshResource->mVertexList);
}

void ModelLoader::normalizeBoneWeights(std::vector<Vertex>& pVertexList) {
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
