#ifndef LAKOT_GLMCONVERTER_H
#define LAKOT_GLMCONVERTER_H

#include <string>

#include <glm/glm.hpp>

class GLMConverter
{
public:
    static std::string toString(const glm::mat4& pMat4);

private:
    GLMConverter();
};

#endif
