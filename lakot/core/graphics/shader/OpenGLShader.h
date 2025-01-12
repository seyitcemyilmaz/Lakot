#ifndef LAKOT_OPENGLSHADER_H
#define LAKOT_OPENGLSHADER_H

#include <lakot/abstract/graphics/shader/AShader.h>

namespace lakot {

class OpenGLShader : public AShader
{
public:
    virtual ~OpenGLShader();
    OpenGLShader(const std::string& pFileContent, ShaderType pType);

    void initialize() override;
    void deinitialize() override;

private:
    bool mIsInitialized;

    std::string mFileContent;
};

}

#endif
