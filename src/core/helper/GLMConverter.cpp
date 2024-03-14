#include "GLMConverter.h"

GLMConverter::GLMConverter()
{

}

std::string GLMConverter::toString(const glm::mat4& pMat4)
{
    std::string tReturnValue = "\n";

    for(unsigned int i = 0; i < 4; i++)
    {
        for(unsigned int  j = 0; j < 4; j++)
        {
            tReturnValue += std::to_string(pMat4[i][j]) + " ";
        }
        tReturnValue += "\n";
    }

    return tReturnValue;
}
