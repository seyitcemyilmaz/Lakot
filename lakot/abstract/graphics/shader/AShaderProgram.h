#ifndef LAKOT_ASHADERPROGRAM_H
#define LAKOT_ASHADERPROGRAM_H

#include <map>

#include "AShader.h"

namespace lakot {

class AShaderProgram : public Object
{
public:
    ~AShaderProgram() override;
    AShaderProgram(const std::string& pName, AShader* pVertexShader, AShader* pFragmentShader);

    virtual void bind() = 0;
    virtual ShaderUniform* createUniform(const std::string& pName) = 0;

    const std::string& getName();

    virtual void setBool(const std::string& pUniformName, bool pValue) = 0;
    virtual void setInt(const std::string& pUniformName, int pValue) = 0;
    virtual void setVec3(const std::string& pUniformName, const glm::vec3& pValue) = 0;
    virtual void setMat4(const std::string& pUniformName, const glm::mat4& pValue) = 0;
    virtual void setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount) = 0;

    virtual void setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId) = 0;

protected:
    std::string mName;

    int mId;

    AShader* mVertexShader;
    AShader* mFragmentShader;

    std::map<std::string, ShaderUniform*> mUniforms;

    ShaderUniform* getUniform(const std::string& pName);
};

}

#endif
