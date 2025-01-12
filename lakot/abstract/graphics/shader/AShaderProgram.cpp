#include "AShaderProgram.h"

using namespace lakot;

AShaderProgram::~AShaderProgram()
{

}

AShaderProgram::AShaderProgram(const std::string& pName, AShader* pVertexShader, AShader* pFragmentShader)
    : Object()
    , mName(pName)
    , mId(-1)
    , mVertexShader(pVertexShader)
    , mFragmentShader(pFragmentShader)
{

}

const std::string& AShaderProgram::getName()
{
    return mName;
}

ShaderUniform* AShaderProgram::getUniform(const std::string& pName)
{
    auto tIterator = mUniforms.find(pName);

    if (tIterator != mUniforms.end())
    {
        return mUniforms[pName];
    }

    ShaderUniform* tUniform = createUniform(pName);

    return tUniform;
}
