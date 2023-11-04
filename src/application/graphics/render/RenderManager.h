#ifndef LAKOT_RENDERER_H
#define LAKOT_RENDERER_H

#include <glm/glm.hpp>

#include "../model/Model.h"
#include "helper/shader/IShader.h"

#define LAKOT_DEFAULT_FAR_PLANE				1000000.0
#define LAKOT_DEFAULT_NEAR_PLANE			0.01

class RenderManager {
private:
	static RenderManager* mInstance;

	double mFarPlaneDistance;
	double mNearPlaneDistance;

	glm::mat4 getProjectionMatrix();
	glm::mat4 getViewMatrix();

	RenderManager();

	void renderMesh(Model* pModel, Mesh* pMesh, IShader* pShader);
	void useMaterial(Model* pModel, Mesh* pMesh, IShader* pShader);

public:
	static RenderManager* getInstance();

	void renderScene();
	void renderGUI();
	void renderModel(Model* pModel, IShader* pShader);
};

#endif
