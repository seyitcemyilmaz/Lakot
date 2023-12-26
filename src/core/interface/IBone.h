#ifndef IBONE_H
#define IBONE_H

#include <string>

#include <glm/glm.hpp>

class IBone {
public:
    IBone();

    virtual const std::string& getName() const = 0;

    virtual const glm::mat4& getTransformationMatrix() const = 0;
    virtual void setTransformationMatrix(const glm::mat4& pTransformationMatrix) = 0;
};

#endif // IBONE_H
