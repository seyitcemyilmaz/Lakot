#include "ShaderModel.h"

ShaderModel::~ShaderModel()
{

}

ShaderModel::ShaderModel(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath)
    : IShader(pShaderName, pVertexShaderFilePath, pFragmentShaderFilePath)
{

    addShaderVariable(ShaderVariableName::eProjection, new ShaderVariable("projection", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eView, new ShaderVariable("view", ShaderVariableDataType::eMat4));
    addShaderVariable(ShaderVariableName::eModel, new ShaderVariable("model", ShaderVariableDataType::eMat4));

    addShaderVariable(ShaderVariableName::eBoneTransformations, new ShaderVariable("boneTransformations", ShaderVariableDataType::eMat4Array));

    addShaderVariable(ShaderVariableName::eAnimationType, new ShaderVariable("animationType", ShaderVariableDataType::eInt));

    addShaderVariable(ShaderVariableName::eDiffuseTexture, new ShaderVariable("material.diffuseTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eNormalsTexture, new ShaderVariable("material.normalsTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eSpecularTexture, new ShaderVariable("material.specularTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eEmissiveTexture, new ShaderVariable("material.emissiveTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eAmbientTexture, new ShaderVariable("material.ambientTexture", ShaderVariableDataType::eSampler2D));
    addShaderVariable(ShaderVariableName::eMetalnessTexture, new ShaderVariable("material.metalnessTexture", ShaderVariableDataType::eSampler2D));

    addShaderVariable(ShaderVariableName::eHasDiffuseTexture, new ShaderVariable("material.hasDiffuseTexture", ShaderVariableDataType::eBool));
    addShaderVariable(ShaderVariableName::eHasNormalsTexture, new ShaderVariable("material.hasNormalsTexture", ShaderVariableDataType::eBool));
    addShaderVariable(ShaderVariableName::eHasSpecularTexture, new ShaderVariable("material.hasSpecularTexture", ShaderVariableDataType::eBool));
    addShaderVariable(ShaderVariableName::eHasEmissiveTexture, new ShaderVariable("material.hasEmissiveTexture", ShaderVariableDataType::eBool));
    addShaderVariable(ShaderVariableName::eHasAmbientTexture, new ShaderVariable("material.hasAmbientTexture", ShaderVariableDataType::eBool));
    addShaderVariable(ShaderVariableName::eHasMetalnessTexture, new ShaderVariable("material.hasMetalnessTexture", ShaderVariableDataType::eBool));

    addShaderVariable(ShaderVariableName::eDiffuseColor, new ShaderVariable("material.diffuseColor", ShaderVariableDataType::eVec3));
    addShaderVariable(ShaderVariableName::eSpecularColor, new ShaderVariable("material.specularColor", ShaderVariableDataType::eVec3));
    addShaderVariable(ShaderVariableName::eEmissiveColor, new ShaderVariable("material.emissiveColor", ShaderVariableDataType::eVec3));
    addShaderVariable(ShaderVariableName::eAmbientColor, new ShaderVariable("material.ambientColor", ShaderVariableDataType::eVec3));
}
