#include "RenderManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include "core/helper/shader/ShaderManager.h"

#include "core/helper/WindowManager.h"
#include "core/helper/camera/CameraManager.h"

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
    IShader* tShader = ShaderManager::getInstance()->getShader(ShaderName::eShader);
    ShaderManager::getInstance()->bindShader(tShader);

	glm::mat4 tProjectionMatrix = getProjectionMatrix();
	glm::mat4 tViewMatrix = getViewMatrix();

    tShader->getShaderVariable(ShaderVariableName::eProjection)->setMat4(tProjectionMatrix);
	tShader->getShaderVariable(ShaderVariableName::eView)->setMat4(tViewMatrix);
}

void RenderManager::renderGUI() { }

glm::mat4 RenderManager::getProjectionMatrix() {
	Camera* tActiveCamera = CameraManager::getInstance()->getActiveCamera();
	double tZoom = tActiveCamera->getZoom();

	int tWindowHeight = WindowManager::getInstance()->getWindowHeight();
	int tWindowWidth = WindowManager::getInstance()->getWindowWidth();

	return glm::perspective(glm::radians(tZoom),
							(double) tWindowWidth / (double) tWindowHeight,
							mNearPlaneDistance,
							mFarPlaneDistance);
}

glm::mat4 RenderManager::getViewMatrix() {
	Camera* tActiveCamera = CameraManager::getInstance()->getActiveCamera();

	glm::vec3 tCameraPosition = tActiveCamera->getPosition();
	glm::vec3 tCameraFront = tActiveCamera->getFrontVector();
	glm::vec3 tCameraUp = tActiveCamera->getUpVector();

	return glm::lookAt(tCameraPosition, tCameraPosition + tCameraFront, tCameraUp);
}
