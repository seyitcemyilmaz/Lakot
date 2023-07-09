#ifndef LAKOT_RENDERER_H
#define LAKOT_RENDERER_H

#include <glm/glm.hpp>

#define LAKOT_DEFAULT_FAR_PLANE				100.0
#define LAKOT_DEFAULT_NEAR_PLANE			0.1

class RenderManager {
private:
	static RenderManager* mInstance;

	double mFarPlaneDistance;
	double mNearPlaneDistance;

	glm::mat4 getProjectionMatrix();
	glm::mat4 getViewMatrix();

	RenderManager();

public:
	static RenderManager* getInstance();

	void renderScene();
	void renderGUI();
};

#endif
