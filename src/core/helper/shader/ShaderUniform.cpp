#include "ShaderUniform.h"

ShaderUniform::ShaderUniform(const std::string& pName)
    : mName(pName)
    , mLocation(-1)
{

}

const std::string& ShaderUniform::getName() const
{
    return mName;
}

int ShaderUniform::getLocation() const
{
    return mLocation;
}

void ShaderUniform::setLocation(int pLocation)
{
    mLocation = pLocation;
}
