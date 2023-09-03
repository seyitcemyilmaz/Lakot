#include "RenderManager.h"

#include "platform/Platform.h"

#include "core/helper/camera/CameraManager.h"
#include "core/helper/shader/ShaderManager.h"
#include "core/helper/window/WindowManager.h"

RenderManager* RenderManager::mInstance = nullptr;

RenderManager* RenderManager::getInstance() {
	if (!mInstance) {
		mInstance = new RenderManager();
	}

	return mInstance;
}

RenderManager::RenderManager() {
	mNearPlaneDistance = LAKOT_DEFAULT_NEAR_PLANE;
	mFarPlaneDistance = LAKOT_DEFAULT_FAR_PLANE;
}

void RenderManager::renderScene() {
	IShader* tShader = ShaderManager::getInstance()->getShader(ShaderName::eModelShader);
	ShaderManager::getInstance()->bindShader(tShader);

	const glm::mat4& tProjectionMatrix = getProjectionMatrix();
	const glm::mat4& tViewMatrix = getViewMatrix();

	tShader->getShaderVariable(ShaderVariableName::eProjection)->setMat4(tProjectionMatrix);
	tShader->getShaderVariable(ShaderVariableName::eView)->setMat4(tViewMatrix);


}

void RenderManager::renderGUI() { }

glm::mat4 RenderManager::getProjectionMatrix() {
	Camera* tActiveCamera = CameraManager::getInstance()->getActiveCamera();
	double tZoom = tActiveCamera->getZoom();

	int tWindowHeight = WindowManager::getInstance()->getWindowHeight();
	int tWindowWidth = WindowManager::getInstance()->getWindowWidth();

	return glm::perspective(glm::radians(tZoom), (double) tWindowWidth / (double) tWindowHeight, mNearPlaneDistance, mFarPlaneDistance);
}

glm::mat4 RenderManager::getViewMatrix() {
	Camera* tActiveCamera = CameraManager::getInstance()->getActiveCamera();

	glm::vec3 tCameraPosition = tActiveCamera->getPosition();
	glm::vec3 tCameraFront = tActiveCamera->getFrontVector();
	glm::vec3 tCameraUp = tActiveCamera->getUpVector();

	return glm::lookAt(tCameraPosition, tCameraPosition + tCameraFront, tCameraUp);
}

void RenderManager::renderModel(Model* pModel, IShader* pShader) {
	const std::vector<Mesh*> pMeshes = pModel->getMeshes();
	size_t tMeshCount = pMeshes.size();

	for (size_t i = 0; i < tMeshCount; i++) {
		renderMesh(pModel, pMeshes[i], pShader);
	}
}

void RenderManager::renderMesh(Model* pModel, Mesh* pMesh, IShader* pShader) {
	useMaterial(pModel, pMesh, pShader);

	glBindVertexArray(pMesh->getMeshResource()->getVAO());
	glDrawElements(GL_TRIANGLES, pMesh->getMeshResource()->getIndiceCount(), GL_UNSIGNED_INT, nullptr);
}

void RenderManager::useMaterial(Model* pModel, Mesh* pMesh, IShader* pShader) {
	MaterialResource* tMaterialResource = pModel->getModelResource()->getMaterialResource(pMesh->getMaterialIndex());

	if (tMaterialResource) {
		TextureResource* tDiffuseTexture = tMaterialResource->getDiffuseTexture();

		unsigned int tUnit = 0;

		if (tDiffuseTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasDiffuseTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eDiffuseTexture)->setTexture(tUnit++, tDiffuseTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasDiffuseTexture)->setBool(false);
			pShader->getShaderVariable(ShaderVariableName::eDiffuseColor)->setVec3(tMaterialResource->getDiffuseColor());
			tUnit++;
		}

		TextureResource* tNormalsTexture = tMaterialResource->getNormalsTexture();

		if (tNormalsTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasNormalsTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eNormalsTexture)->setTexture(tUnit++, tNormalsTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasNormalsTexture)->setBool(false);
			tUnit++;
		}

		TextureResource* tSpecularTexture = tMaterialResource->getSpecularTexture();

		if (tSpecularTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasSpecularTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eSpecularTexture)->setTexture(tUnit++, tSpecularTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasSpecularTexture)->setBool(false);
			pShader->getShaderVariable(ShaderVariableName::eSpecularColor)->setVec3(tMaterialResource->getSpecularColor());
			tUnit++;
		}

		TextureResource* tEmissiveTexture = tMaterialResource->getEmissiveTexture();

		if (tEmissiveTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasEmissiveTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eEmissiveTexture)->setTexture(tUnit++, tEmissiveTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasEmissiveTexture)->setBool(false);
			pShader->getShaderVariable(ShaderVariableName::eEmissiveColor)->setVec3(tMaterialResource->getEmissiveColor());
			tUnit++;
		}

		TextureResource* tAmbientTexture = tMaterialResource->getAmbientTexture();

		if (tAmbientTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasAmbientTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eAmbientTexture)->setTexture(tUnit++, tAmbientTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasAmbientTexture)->setBool(false);
			pShader->getShaderVariable(ShaderVariableName::eAmbientColor)->setVec3(tMaterialResource->getAmbientColor());
			tUnit++;
		}

		TextureResource* tMetalnessTexture = tMaterialResource->getMetalnessTexture();

		if (tMetalnessTexture) {
			pShader->getShaderVariable(ShaderVariableName::eHasMetalnessTexture)->setBool(true);
			pShader->getShaderVariable(ShaderVariableName::eMetalnessTexture)->setTexture(tUnit++, tMetalnessTexture->getTextureId());
		}
		else {
			pShader->getShaderVariable(ShaderVariableName::eHasMetalnessTexture)->setBool(false);
			tUnit++;
		}
	}
}
