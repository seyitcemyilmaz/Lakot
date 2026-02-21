#ifndef LAKOT_SHADER_MANAGER_H
#define LAKOT_SHADER_MANAGER_H

#include <unordered_map>
#include <string>

#include "ShaderProgram.h"

namespace lakot
{

class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    void initialize();
    void deinitialize();

    ShaderProgram* createProgram(const std::string& pName, const std::string& pVertexSource, const std::string& pFragmentSource);

    ShaderProgram* getProgram(const std::string& pName) const;

private:
    std::unordered_map<std::string, ShaderProgram*> mPrograms;
};

}

#endif
