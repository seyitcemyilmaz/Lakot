#ifndef LAKOT_SHADERMANAGER_H
#define LAKOT_SHADERMANAGER_H

#include "shader.h"

class ShaderManager {
private:
    static ShaderManager* mInstance;

    std::map<std::string, Shader*> mShaders;
    std::pair<std::string, Shader*> mActiveShader;

    void setActiveShaderNull();

    ShaderManager();
public:
    static ShaderManager* getInstance();

    void addShader(std::string pShaderName, Shader* pShader);
    void bindShader(std::string pShaderName);

    void deleteShaders();
};

#endif
