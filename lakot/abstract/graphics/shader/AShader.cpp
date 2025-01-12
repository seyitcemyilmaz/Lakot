#include "AShader.h"

using namespace lakot;

AShader::~AShader()
{

}

AShader::AShader()
    : Object()
    , mType(ShaderType::eUndefined)
    , mId(UINT_MAX)
{

}

ShaderType AShader::getType() const noexcept
{
    return mType;
}

unsigned int AShader::getId() const
{
    return mId;
}
