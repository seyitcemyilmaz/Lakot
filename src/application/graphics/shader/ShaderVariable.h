#ifndef LAKOT_SHADERVARIABLE_H
#define LAKOT_SHADERVARIABLE_H

#include <string>

#include <glm/glm.hpp>

enum class ShaderVariableName {
    eView,
    eProjection,
    eModel
};

enum class ShaderVariableDataType {
    eMat4,
    eVec3,
};

class ShaderVariable {
private:
    std::string mName;

    ShaderVariableDataType mDataType;

    int mLocation = -1;
public:
    ShaderVariable(const std::string& pName, ShaderVariableDataType pDataType);

    const std::string& getName();

    int getLocation() const;
    void setLocation(int pLocation);

    void setMat4(const glm::mat4& pValue) const;
};

#endif // SHADERVARIABLE_H
