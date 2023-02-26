#ifndef LAKOT_SHADER_H
#define LAKOT_SHADER_H

#include "core/platform.h"

class Shader {
private:
    unsigned int mShaderProgramId;

    std::string mVertexShaderFilePath;
    std::string mFragmentShaderFilePath;

    void createShaderProgram(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
    unsigned int compileShader(std::string pShaderFilePath, unsigned int tShaderType);
public:
    Shader();
    Shader(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
    ~Shader();

    void bind();
};

#endif
