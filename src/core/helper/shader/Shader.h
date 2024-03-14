#ifndef LAKOT_SHADER_H
#define LAKOT_SHADER_H

#include <string>

enum class ShaderType
{
    eNull = 0,
    eVertex = 1,
    eFragment = 2,
    eGeometry = 3
};

class Shader
{
public:
    virtual ~Shader();
    Shader(const std::string& pFileLocation, ShaderType pType);

    void compile();

    unsigned int getId() const;

    const std::string& getFileLocation() const;
    const std::string& getContent() const;

    bool getIsCompiled() const;

private:
    unsigned int mId;

    ShaderType mType;

    std::string mContent;
    std::string mFileLocation;

    bool mIsCompiled;

    void createShaderProgram();
    unsigned int compileShader(const std::string& pShaderFilePath, unsigned int tShaderType);
};

#endif
