#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <spdlog/spdlog.h>

#include "platform/Platform.h"

#include "helper/AssimpToGLMHelper.h"

#include "BoneWeightLoader.h"
#include "MaterialLoader.h"
#include "MeshLoader.h"
#include "AnimationLoader.h"

ModelLoader::ModelLoader(const std::string& pModelPath)
	: mModelPath(pModelPath) {
	mModelImportFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs | aiProcess_SplitByBoneCount;

	mModelResource = nullptr;
}

ModelResource* ModelLoader::loadModel() {
	Assimp::Importer tImporter;

	tImporter.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* tScene = tImporter.ReadFile(mModelPath, mModelImportFlags);

	if (!tScene) {
		tImporter.FreeScene();
		delete tScene;
		return nullptr;
	}

	applyAxisCorrection(tScene);

	mModelResource = new ModelResource();

	mModelResource->setGlobalInverseTransform(AssimpToGLMHelper::toMat4(tScene->mRootNode->mTransformation));

	processNode(tScene->mRootNode, tScene, nullptr);

	processAnimations(tScene);

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
		tMaterialLoader = new MaterialLoader(mModelResource, tMaterial, pScene, mModelPath);

		MaterialResource* tMaterialResource = tMaterialLoader->loadMaterial();
		mModelResource->mMaterialResources.push_back(tMaterialResource);

		delete tMaterialLoader;
	}
}

void ModelLoader::processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource) {
	std::string tNodeName = pNode->mName.C_Str();
	glm::mat4 tTransformationMatrix = AssimpToGLMHelper::toMat4(pNode->mTransformation);

	NodeResource* tNodeResource = new NodeResource(tNodeName, pParentNodeResource, tTransformationMatrix);

	mModelResource->mNodeResources.push_back(tNodeResource);

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
	MeshLoader tMeshLoader(pMesh, mModelResource);
	MeshResource* tMeshResource = tMeshLoader.loadMesh();

	if (pMesh->HasBones()) {
		extractBones(pMesh, tMeshResource);
		tMeshResource->setHasBone(true);
	}

	createMeshBuffers(tMeshResource);

	tMeshResource->setConnectedNode(pConnectedNodeResource);
	pConnectedNodeResource->addChildMesh(tMeshResource);

	mModelResource->addMeshResource(tMeshResource);
}

void ModelLoader::processAnimations(const aiScene* pScene) {
	unsigned int tAnimationCount = pScene->mNumAnimations;

	for (unsigned int i = 0; i < tAnimationCount; i++) {
		aiAnimation* tAnimation = pScene->mAnimations[i];

		AnimationLoader tAnimationLoader(mModelResource, tAnimation);
		AnimationResource* tAnimationResource = tAnimationLoader.load();

		mModelResource->mAnimationResources.push_back(tAnimationResource);
	}
}

void ModelLoader::createMeshBuffers(MeshResource* pMeshResource) {
	glGenVertexArrays(1, &pMeshResource->mVAO);
	glGenBuffers(1, &pMeshResource->mVBO);
	glGenBuffers(1, &pMeshResource->mIBO);

	glBindVertexArray(pMeshResource->mVAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMeshResource->mIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMeshResource->mIndexList.size() * sizeof(unsigned int), pMeshResource->mIndexList.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, pMeshResource->mVBO);
	glBufferData(GL_ARRAY_BUFFER, pMeshResource->mVertexList.size() * sizeof(Vertex), pMeshResource->mVertexList.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, textureCoordinates)));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, boneIds)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, boneWeights)));

	pMeshResource->mVertexList.clear();
	pMeshResource->mIndexList.clear();

	glBindVertexArray(0);
}

void ModelLoader::applyAxisCorrection(const aiScene* pScene) {
	// From: https://github.com/assimp/assimp/issues/849#issuecomment-875475292
	if (pScene->mMetaData) {
		int UpAxis = 1;
		int UpAxisSign = 1;
		int FrontAxis = 2;
		int FrontAxisSign = 1;
		int CoordAxis = 0;
		int CoordAxisSign = 1;

		double UnitScaleFactor = 1.0;

		for (unsigned tMetaDataIndex = 0; tMetaDataIndex < pScene->mMetaData->mNumProperties; tMetaDataIndex++) {
			const char* tStr = pScene->mMetaData->mKeys[tMetaDataIndex].C_Str();

			if (strcmp(tStr, "UpAxis") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, UpAxis);
				spdlog::info("UpAxis: {0}", UpAxis);
			}

			if (strcmp(tStr, "UpAxisSign") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, UpAxisSign);
				spdlog::info("UpAxisSign: {0}", UpAxisSign);
			}

			if (strcmp(tStr, "FrontAxis") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, FrontAxis);
				spdlog::info("FrontAxis: {0}", FrontAxis);
			}

			if (strcmp(tStr, "FrontAxisSign") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, FrontAxisSign);
				spdlog::info("FrontAxisSign: {0}", FrontAxisSign);
			}

			if (strcmp(tStr, "CoordAxis") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, CoordAxis);
				spdlog::info("CoordAxis: {0}", CoordAxis);
			}

			if (strcmp(tStr, "CoordAxisSign") == 0) {
				pScene->mMetaData->Get<int>(tMetaDataIndex, CoordAxisSign);
				spdlog::info("CoordAxisSign: {0}", CoordAxisSign);
			}

			if (strcmp(tStr, "UnitScaleFactor") == 0) {
				pScene->mMetaData->Get<double>(tMetaDataIndex, UnitScaleFactor);
				spdlog::info("UnitScaleFactor: {0}", UnitScaleFactor);
			}
		}

		/// Explanation: Sign should be -1 or 1, it shows direction of the axis that is used.
		/// UpAxis, FrontAxis, CoordAxis can be 0, 1, 2. 0 is x axis, 1 is y axis, 2 is z axis.

		aiVector3D tUpVector, tForwardVector, tRightVector;

		tUpVector[UpAxis] = UpAxisSign * (float) UnitScaleFactor;
		tForwardVector[FrontAxis] = FrontAxisSign * (float) UnitScaleFactor;
		tRightVector[CoordAxis] = CoordAxisSign * (float) UnitScaleFactor;

		aiMatrix4x4 tMat(tRightVector.x, tRightVector.y, tRightVector.z, 0.0f,
						tUpVector.x, tUpVector.y, tUpVector.z, 0.0f,
						tForwardVector.x, tForwardVector.y, tForwardVector.z, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f);

		pScene->mRootNode->mTransformation = tMat; ///TODO: will be checked. Some people say that it should be tAiScene->mRootNode->mTransformation *= tMat;
	}
}

void ModelLoader::extractBones(aiMesh* pMesh, MeshResource* pMeshResource) {
	aiBone** tBones = pMesh->mBones;
	unsigned int tBoneCount = pMesh->mNumBones;

	std::vector<NodeResource*>& tNodeResources = mModelResource->mNodeResources;
	unsigned int tNodeCount = mModelResource->getNodeCount();

	for (unsigned int i = 0; i < tBoneCount; i++) {
		aiBone* tBone = tBones[i];
		std::string tBoneName = tBone->mName.C_Str();

		NodeResource* tNodeResource = nullptr;

		for (unsigned int j = 0; j < tNodeCount; j++) {
			if (tNodeResources[j]->getName() == tBoneName) {
				tNodeResource = tNodeResources[j];
				break;
			}
		}

		if (!tNodeResource) {
			spdlog::warn("Node: {0} is not found", tBoneName);
			continue;
		}

		BoneResource* tBoneResource = tNodeResource->getBone();

		if (!tBoneResource) {
			BoneResource* tBoneResource = new BoneResource(tBoneName, AssimpToGLMHelper::toMat4(tBone->mOffsetMatrix));
			mModelResource->addBoneResource(tBoneResource);
			tNodeResource->setBoneResource(tBoneResource);
		}

		unsigned int tBoneId = mModelResource->getBoneId(tBoneResource);

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
