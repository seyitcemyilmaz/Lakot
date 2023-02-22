#ifndef LAKOT_SHADERMANAGER_H
#define LAKOT_SHADERMANAGER_H

#include "shader.h"

class ShaderManager {
private:
    std::map<std::string, Shader*> mShaders;

    Shader* mActiveShader;
public:
    ShaderManager();
    ~ShaderManager();

    void addShader(std::string pShaderName, Shader* pShader);
    void bindShader(std::string pShaderName);
    void bindShader(Shader* pShader);
};

#endif
