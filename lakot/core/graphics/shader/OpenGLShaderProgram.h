#ifndef LAKOT_OPENGLSHADERPROGRAM_H
#define LAKOT_OPENGLSHADERPROGRAM_H

#include <lakot/abstract/graphics/shader/AShaderProgram.h>

#include "OpenGLShader.h"

namespace lakot {

class OpenGLShaderProgram final : public AShaderProgram
{
public:
    virtual ~OpenGLShaderProgram() override;
    OpenGLShaderProgram(const std::string& pName, OpenGLShader* pVertexShader, OpenGLShader* pFragmentShader);

    void initialize() override;
    void deinitialize() override;

    void bind() override;

    void setBool(const std::string& pUniformName, bool pValue) override;
    void setInt(const std::string& pUniformName, int pValue) override;
    void setVec3(const std::string& pUniformName, const glm::vec3& pValue) override;
    void setMat4(const std::string& pUniformName, const glm::mat4& pValue) override;
    void setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount) override;

    void setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId) override;

private:
    ShaderUniform* createUniform(const std::string& pName) override;
};

}

#endif
