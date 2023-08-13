#ifndef LAKOT_SHADER_H
#define LAKOT_SHADER_H

#include "core/platform/Platform.h"

class Shader {
private:
	unsigned int mShaderProgramId;

	std::string mVertexShaderFilePath;
	std::string mFragmentShaderFilePath;

	int mProjectionMatrixLocation;
	int mViewMatrixLocation;
	int mModelMatrixLocation;

	void createShaderProgram(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
	unsigned int compileShader(const std::string& pShaderFilePath, unsigned int tShaderType);

public:
	Shader(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
	~Shader();

	void bind() const;

	int getProjectionMatrixLocation() const;
	int getViewMatrixLocation() const;
	int getModelMatrixLocation() const;
};

#endif
