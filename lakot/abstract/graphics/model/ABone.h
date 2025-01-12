#ifndef LAKOT_ABONE_H
#define LAKOT_ABONE_H

#include <string>

#include <glm/glm.hpp>

#include <lakot/utilities/Object.h>

namespace lakot {

class ABone : public Object
{
public:
    virtual ~ABone() override;
    ABone(const glm::mat4& pTransformation);

    virtual const std::string& getName() const = 0;

    glm::mat4& getTransformation();

protected:
    glm::mat4 mTransformation;
};

}

#endif
