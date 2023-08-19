#ifndef LAKOT_SHADER_H
#define LAKOT_SHADER_H

#include "core/helper/shader/IShader.h"

class ShaderModel final : public IShader {
public:
    ~ShaderModel();
    ShaderModel(ShaderName pShaderName, const char* pVertexShaderFilePath, const char* pFragmentShaderFilePath);
};

#endif
