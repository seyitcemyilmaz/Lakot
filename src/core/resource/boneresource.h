#ifndef LAKOT_BONERESOURCE_H
#define LAKOT_BONERESOURCE_H

#include <iostream>

#include <glm/glm.hpp>

class BoneResource {
private:
    std::string mName;

    glm::mat4 mOffsetMatrix;
public:
    BoneResource(std::string pName, glm::mat4 pOffsetMatrix);
};

#endif
