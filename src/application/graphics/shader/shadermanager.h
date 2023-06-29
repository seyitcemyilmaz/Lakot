#ifndef LAKOT_SHADERMANAGER_H
#define LAKOT_SHADERMANAGER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

class ShaderManager {
private:
	static ShaderManager* mInstance;

	std::map<std::string, Shader*> mShaders;
	std::pair<std::string, Shader*> mActiveShader;

	void setActiveShaderNull();

	ShaderManager();
public:
	static ShaderManager* getInstance();

	void addShader(std::string pShaderName, Shader* pShader);
	void bindShader(std::string pShaderName);

	void deleteShaders();

	void setProjectionMatrix(glm::mat4& tProjectionMatrix);
	void setViewMatrix(glm::mat4& tViewMatrix);
	void setModelMatrix(glm::mat4& tModelMatrix);
};

#endif
