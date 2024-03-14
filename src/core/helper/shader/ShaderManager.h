#ifndef LAKOT_SHADERMANAGER_H
#define LAKOT_SHADERMANAGER_H

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShaderProgram.h"

class ShaderManager
{
public:
    static ShaderManager* getInstance();

    ShaderProgram* getShaderProgram(const std::string& pShaderProgramName);

    void addShaderProgram(ShaderProgram* pShaderProgram);
    void bindShaderProgram(ShaderProgram* pShaderProgram);

    void deleteShaderPrograms();

private:
    static ShaderManager* mInstance;

    std::vector<ShaderProgram*> mShaderPrograms;
    ShaderProgram* mActiveShaderProgram;

    void setActiveShaderNull();

    ShaderManager();
};

#endif
