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

void MeshResource::draw() {
	MaterialResource* tMaterialResource = mModelResource->getMaterialResource(mMaterialIndex);

    // TODO: CHECK glUniform1i(_textureUniform, 0);
    // https://stackoverflow.com/questions/9661878/set-the-texture-for-by-gluniform1i

    if (tMaterialResource) {
		TextureResource* tDiffuseTexture = tMaterialResource->getDiffuseTexture();

        glActiveTexture(GL_TEXTURE0);

        if (tDiffuseTexture) {
            glBindTexture(GL_TEXTURE_2D, tDiffuseTexture->getTextureId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        TextureResource* tNormalsTexture = tMaterialResource->getNormalsTexture();

        glActiveTexture(GL_TEXTURE1);

        if (tNormalsTexture) {
            glBindTexture(GL_TEXTURE_2D, tNormalsTexture->getTextureId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        TextureResource* tSpecularTexture = tMaterialResource->getSpecularTexture();

        glActiveTexture(GL_TEXTURE2);

        if (tSpecularTexture) {
            glBindTexture(GL_TEXTURE_2D, tSpecularTexture->getTextureId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        TextureResource* tEmissiveTexture = tMaterialResource->getEmissiveTexture();

        glActiveTexture(GL_TEXTURE3);

        if (tEmissiveTexture) {
            glBindTexture(GL_TEXTURE_2D, tEmissiveTexture->getTextureId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        TextureResource* tAmbientTexture = tMaterialResource->getAmbientTexture();

        glActiveTexture(GL_TEXTURE4);

        if (tAmbientTexture) {
            glBindTexture(GL_TEXTURE_2D, tAmbientTexture->getTextureId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
	}

	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, nullptr);
	glActiveTexture(GL_TEXTURE0);
}
