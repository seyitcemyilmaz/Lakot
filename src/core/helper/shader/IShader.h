#ifndef LAKOT_ISHADER_H
#define LAKOT_ISHADER_H

#include <map>
#include <string>

#include "ShaderVariable.h"

enum class ShaderName {
	eNULLShader,
	eShader
};

class IShader {
private:
	unsigned int mShaderProgramId;

	ShaderName mShaderName;

	std::string mVertexShaderFilePath;
	std::string mFragmentShaderFilePath;

	std::map<ShaderVariableName, ShaderVariable*> mShaderVariables;

	void createShaderProgram();
	unsigned int compileShader(const std::string& pShaderFilePath, unsigned int tShaderType);

protected:
	unsigned int getShaderProgramId() const;

	void addShaderVariable(ShaderVariableName pShaderVariableName, ShaderVariable* pShaderVariable);

	void bind() const;
	friend class ShaderManager;

public:
	IShader(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
	virtual ~IShader();

	ShaderName getShaderName();

	ShaderVariable* getShaderVariable(ShaderVariableName pShaderVariableName);
};

#endif // ISHADER_H
