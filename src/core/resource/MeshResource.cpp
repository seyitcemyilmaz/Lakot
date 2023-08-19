#include "MeshResource.h"

#include "ModelResource.h"

#include "platform/Platform.h"

MeshResource::MeshResource(const std::string& pName, ModelResource* pModelResource, const std::vector<Vertex>& pVertexList,
						   const std::vector<unsigned int>& pIndexList, unsigned int pMaterialIndex) :
	mName(pName), mModelResource(pModelResource), mVertexList(pVertexList), 
	mIndexList(pIndexList), mMaterialIndex(pMaterialIndex) { }

std::string MeshResource::getName() const {
	return mName;
}

unsigned int MeshResource::getMaterialIndex() const {
	return mMaterialIndex;
}

void MeshResource::setHasBone(bool pHasBone) {
	mHasBone = pHasBone;
}

void MeshResource::setConnectedNode(NodeResource* pConnectedNode) {
	mConnectedNode = pConnectedNode;
}

void MeshResource::createBuffers() {
	mIndexCount = static_cast<int>(mIndexList.size());

	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mIBO);

	glBindVertexArray(mVAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndexList.size() * sizeof(unsigned int), mIndexList.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mVertexList.size() * sizeof(Vertex), mVertexList.data(), GL_STATIC_DRAW);

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

	glBindVertexArray(0);
}

void MeshResource::draw(IShader* pShader) {
	MaterialResource* tMaterialResource = mModelResource->getMaterialResource(mMaterialIndex);

    if (tMaterialResource) {
		TextureResource* tDiffuseTexture = tMaterialResource->getDiffuseTexture();

        unsigned int tUnit = 0;

        if (tDiffuseTexture) {
            pShader->getShaderVariable(ShaderVariableName::eDiffuseTexture)->setTexture(tUnit++, tDiffuseTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eDiffuseTexture)->setTexture(tUnit++, -1);
        }

        TextureResource* tNormalsTexture = tMaterialResource->getNormalsTexture();

        if (tNormalsTexture) {
            pShader->getShaderVariable(ShaderVariableName::eNormalsTexture)->setTexture(tUnit++, tNormalsTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eNormalsTexture)->setTexture(tUnit++, -1);
        }

        TextureResource* tSpecularTexture = tMaterialResource->getSpecularTexture();

        if (tSpecularTexture) {
            pShader->getShaderVariable(ShaderVariableName::eSpecularTexture)->setTexture(tUnit++, tSpecularTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eSpecularTexture)->setTexture(tUnit++, -1);
        }

        TextureResource* tEmissiveTexture = tMaterialResource->getEmissiveTexture();

        if (tEmissiveTexture) {
            pShader->getShaderVariable(ShaderVariableName::eEmissiveTexture)->setTexture(tUnit++, tEmissiveTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eEmissiveTexture)->setTexture(tUnit++, -1);
        }

        TextureResource* tAmbientTexture = tMaterialResource->getAmbientTexture();

        if (tAmbientTexture) {
            pShader->getShaderVariable(ShaderVariableName::eAmbientTexture)->setTexture(tUnit++, tAmbientTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eAmbientTexture)->setTexture(tUnit++, -1);
        }

        TextureResource* tMetalnessTexture = tMaterialResource->getMetalnessTexture();

        if (tMetalnessTexture) {
            pShader->getShaderVariable(ShaderVariableName::eMetalnessTexture)->setTexture(tUnit++, tMetalnessTexture->getTextureId());
        }
        else {
            pShader->getShaderVariable(ShaderVariableName::eMetalnessTexture)->setTexture(tUnit++, -1);
        }
	}

	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, nullptr);
	glActiveTexture(GL_TEXTURE0);
}
