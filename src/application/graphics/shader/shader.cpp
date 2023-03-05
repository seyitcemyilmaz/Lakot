#include "shader.h"

Shader::Shader(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath) {
    mShaderProgramId = 0;

    createShaderProgram(pVertexShaderFilePath, pFragmentShaderFilePath);

    mProjectionMatrixLocation = glGetUniformLocation(mShaderProgramId, "projection");
    mViewMatrixLocation = glGetUniformLocation(mShaderProgramId, "view");
    mModelMatrixLocation = glGetUniformLocation(mShaderProgramId, "model");
}

Shader::~Shader() {
    glDeleteProgram(mShaderProgramId);
}

void Shader::bind() {
    glUseProgram(mShaderProgramId);
}

void Shader::createShaderProgram(const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath) {
    mVertexShaderFilePath = Platform::getLakotAssetsPath() + "/" + pVertexShaderFilePath;
    mFragmentShaderFilePath = Platform::getLakotAssetsPath() + "/" + pFragmentShaderFilePath;

    unsigned int tVertexShaderId = compileShader(mVertexShaderFilePath, GL_VERTEX_SHADER);
    unsigned int tFragmentShaderId = compileShader(mFragmentShaderFilePath, GL_FRAGMENT_SHADER);

    mShaderProgramId = glCreateProgram();

    glAttachShader(mShaderProgramId, tVertexShaderId);
    glAttachShader(mShaderProgramId, tFragmentShaderId);

    glLinkProgram(mShaderProgramId);

    int tIsShaderProgramLinked = 0;

    glGetProgramiv(mShaderProgramId, GL_LINK_STATUS, &tIsShaderProgramLinked);

    if (!tIsShaderProgramLinked) {
        char tErrorInfo[512];
        glGetProgramInfoLog(mShaderProgramId, 512, nullptr, tErrorInfo);
        throw "Linking error in shader program:" + std::string(tErrorInfo);
    }

    glDeleteShader(tVertexShaderId);
    glDeleteShader(tFragmentShaderId);
}

unsigned int Shader::compileShader(std::string pShaderFilePath, unsigned int tShaderType) {
    unsigned int tShaderId = glCreateShader(tShaderType);

    std::string tShaderFileContent = LAKOT_SHADER_HEADER + Platform::readFileContent(pShaderFilePath);
    const char* tShaderFileContentCStr = tShaderFileContent.c_str();

    glShaderSource(tShaderId, 1, &tShaderFileContentCStr, nullptr);
    glCompileShader(tShaderId);

    int tIsShaderCompiled = 0;

    glGetShaderiv(tShaderId, GL_COMPILE_STATUS, &tIsShaderCompiled);

    if (!tIsShaderCompiled) {
        char tErrorInfo[512];
        glGetShaderInfoLog(tShaderId, 512, nullptr, tErrorInfo);
        throw pShaderFilePath + " does not compiled.";
    }

    return tShaderId;
}

int Shader::getProjectionMatrixLocation() {
    return mProjectionMatrixLocation;
}

int Shader::getViewMatrixLocation() {
    return mViewMatrixLocation;
}

int Shader::getModelMatrixLocation() {
    return mModelMatrixLocation;
}
