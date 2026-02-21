#ifndef LAKOT_SHADER_H
#define LAKOT_SHADER_H

#include <string>

#include "Type.h"

namespace lakot
{

class Shader
{
public:
    virtual ~Shader();
    Shader(const std::string& pFileContent, ShaderType pType);

    void initialize();
    void deinitialize();

    unsigned int getId() const;

private:
    bool mIsInitialized;

    unsigned int mId;

    ShaderType mType;

    std::string mFileContent;
};

}

#endif
