#ifndef LAKOT_SHADERPROGRAM_H
#define LAKOT_SHADERPROGRAM_H

#include <string>
#include <map>

#include <glm/glm.hpp>

#include "Shader.h"
#include "ShaderUniform.h"

class ShaderProgram
{
public:
    virtual ~ShaderProgram();
    ShaderProgram(const std::string& pName, Shader* pVertexShader, Shader* pFragmentShader, Shader* pGeometryShader = nullptr);

    void init();

    void bind();

    const std::string& getName();

    void setBool(const std::string& pUniformName, bool pValue);
    void setInt(const std::string& pUniformName, int pValue);
    void setVec3(const std::string& pUniformName, const glm::vec3& pValue);
    void setMat4(const std::string& pUniformName, const glm::mat4& pValue);
    void setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount);

    void setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId);

private:
    std::string mName;

    unsigned int mId;

    Shader* mVertexShader;
    Shader* mFragmentShader;
    Shader* mGeometryShader;

    std::map<std::string, ShaderUniform*> mUniforms;

    ShaderUniform* getUniform(const std::string& pName);
    ShaderUniform* createUniform(const std::string& pName);
};

#endif
