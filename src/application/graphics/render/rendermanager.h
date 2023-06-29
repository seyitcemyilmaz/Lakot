#ifndef LAKOT_RENDERER_H
#define LAKOT_RENDERER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/platform.h"

#define LAKOT_DEFAULT_FAR_PLANE				100.0f
#define LAKOT_DEFAULT_NEAR_PLANE			0.1f

class RenderManager {
private:
	static RenderManager* mInstance;

	float mFarPlaneDistance;
	float mNearPlaneDistance;

	glm::mat4 getProjectionMatrix();
	glm::mat4 getViewMatrix();

	RenderManager();
public:
	static RenderManager* getInstance();

	void renderScene();
	void renderGUI();
};

#endif
