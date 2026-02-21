#ifndef LAKOT_SHADER_PROGRAM_H
#define LAKOT_SHADER_PROGRAM_H

#include "Shader.h"

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace lakot
{

struct ShaderUniform
{
    ShaderUniform(const std::string& pName)
        : name(pName)
        , location(-1)
    {

    }

    std::string name;
    int location;
};

class ShaderProgram
{
public:
    virtual ~ShaderProgram();
    ShaderProgram(const std::string& pName, Shader* pVertexShader, Shader* pFragmentShader);

    void initialize();
    void deinitialize();

    void bind() const;
    void unbind() const;

    void setBool(const std::string& pUniformName, bool pValue);
    void setInt(const std::string& pUniformName, int pValue);
    void setVec3(const std::string& pUniformName, const glm::vec3& pValue);
    void setMat4(const std::string& pUniformName, const glm::mat4& pValue);
    void setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount);
    void setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId);

private:
    unsigned int mId;
    std::string mName;
    Shader* mVertexShader;
    Shader* mFragmentShader;
    std::unordered_map<std::string, ShaderUniform*> mUniforms;

    ShaderUniform* getUniform(const std::string& pName);
    ShaderUniform* createUniform(const std::string& pName);

};

}

#endif
