#ifndef LAKOT_SHADERMANAGER_H
#define LAKOT_SHADERMANAGER_H

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "IShader.h"

class ShaderManager {
private:
	static ShaderManager* mInstance;

	std::vector<IShader*> mShaders;
	IShader* mActiveShader = nullptr;

	void setActiveShaderNull();

	ShaderManager();

public:
	static ShaderManager* getInstance();

	IShader* getShader(ShaderName pShaderName);

	void addShader(IShader* pShader);
	void bindShader(IShader* pShader);

	void deleteShaders();
};

#endif
