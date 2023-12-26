#include "ShaderVariable.h"

#include <glm/gtc/type_ptr.hpp>

#include "platform/Platform.h"

ShaderVariable::ShaderVariable(const std::string& pName, ShaderVariableDataType pDataType)
    : mName(pName)
    , mDataType(pDataType)
{

}

const std::string& ShaderVariable::getName() const
{
    return mName;
}

void ShaderVariable::setLocation(int pLocation)
{
    mLocation = pLocation;
}

int ShaderVariable::getLocation() const
{
    return mLocation;
}

void ShaderVariable::setBool(bool pValue) const
{
    if (mDataType != ShaderVariableDataType::eBool)
    {
        throw "Invalid data type";
    }

    glUniform1i(mLocation, pValue);
}

void ShaderVariable::setInt(int pValue) const
{
    if (mDataType != ShaderVariableDataType::eInt)
    {
        throw "Invalid data type";
    }

    glUniform1i(mLocation, pValue);
}

void ShaderVariable::setVec3(const glm::vec3& pValue) const
{
    if (mDataType != ShaderVariableDataType::eVec3)
    {
        throw "Invalid data type";
    }

    glUniform3fv(mLocation, 1, glm::value_ptr(pValue));
}

void ShaderVariable::setMat4(const glm::mat4& pValue) const
{
    if (mDataType != ShaderVariableDataType::eMat4)
    {
        throw "Invalid data type";
    }

    glUniformMatrix4fv(mLocation, 1, false, glm::value_ptr(pValue));
}

void ShaderVariable::setMat4Array(const glm::mat4* pArray, unsigned int pCount) const
{
    if (mDataType != ShaderVariableDataType::eMat4Array)
    {
        throw "Invalid data type";
    }

    glUniformMatrix4fv(mLocation, pCount, GL_FALSE, glm::value_ptr(*pArray));
}

void ShaderVariable::setTexture(unsigned int pUnit, int pTextureId) const
{
    if (mDataType != ShaderVariableDataType::eSampler2D)
    {
        throw "Invalid data type";
    }

    if (pTextureId == -1)
    {
        throw "Invalid Texture ID.";
    }

    glActiveTexture(GL_TEXTURE0 + pUnit);
    glBindTexture(GL_TEXTURE_2D, pTextureId);
    glUniform1i(mLocation, pUnit);
}
