#include "ShaderModel.h"

ShaderModel::ShaderModel(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath) :
    IShader(pShaderName, pVertexShaderFilePath, pFragmentShaderFilePath) {

    addShaderVariable(ShaderVariableName::eProjection, new ShaderVariable("projection", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eView, new ShaderVariable("view", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eModel, new ShaderVariable("model", ShaderVariableDataType::eMat4));
}

ShaderModel::~ShaderModel() {
    IShader::~IShader();
}
