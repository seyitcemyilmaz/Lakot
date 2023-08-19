#include "ShaderVariable.h"

#include <glm/gtc/type_ptr.hpp>

#include "platform/Platform.h"

ShaderVariable::ShaderVariable(const std::string& pName, ShaderVariableDataType pDataType) :
    mName(pName), mDataType(pDataType) { }

const std::string& ShaderVariable::getName() {
    return mName;
}

void ShaderVariable::setLocation(int pLocation) {
    mLocation = pLocation;
}

int ShaderVariable::getLocation() const {
    return mLocation;
}

void ShaderVariable::setBool(bool pValue) {
    if (mDataType != ShaderVariableDataType::eBool) {
        throw "Invalid data type";
    }

    glUniform1i(mLocation, pValue);
}

void ShaderVariable::setMat4(const glm::mat4& pValue) const {
    if (mDataType != ShaderVariableDataType::eMat4) {
        throw "Invalid data type";
    }

    glUniformMatrix4fv(mLocation, 1, false, glm::value_ptr(pValue));
}

void ShaderVariable::setTexture(unsigned int pUnit, int pTextureId) const {
    if (mDataType != ShaderVariableDataType::eSampler2D) {
        throw "Invalid data type";
    }

    glActiveTexture(GL_TEXTURE0 + pUnit);
    glBindTexture(GL_TEXTURE_2D, pTextureId);
    glUniform1i(mLocation, pUnit);
}
