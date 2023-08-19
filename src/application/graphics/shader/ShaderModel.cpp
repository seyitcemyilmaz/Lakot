#include "ShaderModel.h"

ShaderModel::ShaderModel(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath) :
    IShader(pShaderName, pVertexShaderFilePath, pFragmentShaderFilePath) {

    addShaderVariable(ShaderVariableName::eProjection, new ShaderVariable("projection", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eView, new ShaderVariable("view", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eModel, new ShaderVariable("model", ShaderVariableDataType::eMat4));

    addShaderVariable(ShaderVariableName::eDiffuseTexture, new ShaderVariable("diffuseTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eNormalsTexture, new ShaderVariable("normalsTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eSpecularTexture, new ShaderVariable("specularTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eEmissiveTexture, new ShaderVariable("emissiveTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eAmbientTexture, new ShaderVariable("ambientTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eMetalnessTexture, new ShaderVariable("metalnessTexture", ShaderVariableDataType::eSampler2D));
}

ShaderModel::~ShaderModel() {
    IShader::~IShader();
}
