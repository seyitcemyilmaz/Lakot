#include "rendermanager.h"

#include "application/graphics/camera/cameramanager.h"
#include "application/graphics/shader/shadermanager.h"
#include "core/helper/windowmanager.h"

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
	ShaderManager* tShaderManager = ShaderManager::getInstance();

	tShaderManager->bindShader("shader");

	glm::mat4 tProjectionMatrix = getProjectionMatrix();
	glm::mat4 tViewMatrix = getViewMatrix();
	glm::mat4 tModelMatrix = glm::mat4(1.0f);

	tShaderManager->setProjectionMatrix(tProjectionMatrix);
	tShaderManager->setViewMatrix(tViewMatrix);
	tShaderManager->setModelMatrix(tModelMatrix);
}

void RenderManager::renderGUI() { }

glm::mat4 RenderManager::getProjectionMatrix() {
	Camera* tActiveCamera = CameraManager::getInstance()->getActiveCamera();
	float tZoom = tActiveCamera->getZoom();

	int tWindowHeight = WindowManager::getInstance()->getWindowHeight();
	int tWindowWidth = WindowManager::getInstance()->getWindowWidth();

	return glm::perspective(glm::radians(tZoom),
							(float) tWindowWidth / (float) tWindowHeight,
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
