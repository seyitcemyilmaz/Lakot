#include "ShaderVariable.h"

#include <glm/gtc/type_ptr.hpp>

#include "core/platform/Platform.h"

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

void ShaderVariable::setMat4(const glm::mat4& pValue) const {
    if (mDataType != ShaderVariableDataType::eMat4) {
        throw "Invalid data type";
    }

    glUniformMatrix4fv(mLocation, 1, false, glm::value_ptr(pValue));
}
