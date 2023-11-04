#include "IShader.h"

#include "helper/FileManager.h"

IShader::~IShader() {
	glDeleteProgram(mShaderProgramId);

	for (auto tShaderVariableIterator = mShaderVariables.begin(); tShaderVariableIterator != mShaderVariables.end(); tShaderVariableIterator++) {
		ShaderVariableName tShaderVariableName = tShaderVariableIterator->first;

		ShaderVariable* tShaderVariable = tShaderVariableIterator->second;
		delete tShaderVariable;

		mShaderVariables[tShaderVariableName] = nullptr;
	}

	mShaderVariables.clear();
}

IShader::IShader(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath)
	: mShaderName(pShaderName)
	, mVertexShaderFilePath(pVertexShaderFilePath)
	, mFragmentShaderFilePath(pFragmentShaderFilePath) {
	createShaderProgram();
}

void IShader::bind() const {
	glUseProgram(mShaderProgramId);
}

ShaderName IShader::getShaderName() {
	return mShaderName;
}

ShaderVariable* IShader::getShaderVariable(ShaderVariableName pShaderVariableName) {
	if (!mShaderVariables.contains(pShaderVariableName)) {
		throw "Shader variable name is not found.";
	}

	return mShaderVariables[pShaderVariableName];
}

unsigned int IShader::getShaderProgramId() const {
	return mShaderProgramId;
}

void IShader::addShaderVariable(ShaderVariableName pShaderVariableName, ShaderVariable* pShaderVariable) {
	if (mShaderVariables.contains(pShaderVariableName)) {
		throw "Shader variable has already created.";
	}

	int tLocation = glGetUniformLocation(mShaderProgramId, pShaderVariable->getName().c_str());
	pShaderVariable->setLocation(tLocation);

	mShaderVariables[pShaderVariableName] = pShaderVariable;
}

void IShader::createShaderProgram() {
	unsigned int tVertexShaderId = compileShader(mVertexShaderFilePath, GL_VERTEX_SHADER);
	unsigned int tFragmentShaderId = compileShader(mFragmentShaderFilePath, GL_FRAGMENT_SHADER);

	mShaderProgramId = glCreateProgram();

	glAttachShader(mShaderProgramId, tVertexShaderId);
	glAttachShader(mShaderProgramId, tFragmentShaderId);

	glLinkProgram(mShaderProgramId);

	int tIsShaderProgramLinked = 0;

	glGetProgramiv(mShaderProgramId, GL_LINK_STATUS, &tIsShaderProgramLinked);

	if (!tIsShaderProgramLinked) {
		char tErrorInfo[512];
		glGetProgramInfoLog(mShaderProgramId, 512, nullptr, tErrorInfo);
		std::cout << tErrorInfo << std::endl;
		throw "Linking error in shader program:" + std::string(tErrorInfo);
	}

	glDeleteShader(tVertexShaderId);
	glDeleteShader(tFragmentShaderId);
}

unsigned int IShader::compileShader(const std::string& pShaderFilePath, unsigned int tShaderType) {
	unsigned int tShaderId = glCreateShader(tShaderType);

	std::string tShaderFileContent = LAKOT_SHADER_HEADER + FileManager::getInstance()->getFileContent(pShaderFilePath);
	const char* tShaderFileContentCStr = tShaderFileContent.c_str();

	glShaderSource(tShaderId, 1, &tShaderFileContentCStr, nullptr);
	glCompileShader(tShaderId);

	int tIsShaderCompiled = 0;

	glGetShaderiv(tShaderId, GL_COMPILE_STATUS, &tIsShaderCompiled);

	if (!tIsShaderCompiled) {
		char tErrorInfo[512];
		glGetShaderInfoLog(tShaderId, 512, nullptr, tErrorInfo);

		std::cout << tErrorInfo << std::endl;
		throw pShaderFilePath + " does not compiled.";
	}

	return tShaderId;
}
