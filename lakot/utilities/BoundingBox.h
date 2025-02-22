#ifndef LAKOT_BOUNDINGBOX_H
#define LAKOT_BOUNDINGBOX_H

#include <glm/glm.hpp>

#include "Object.h"

namespace lakot {

class BoundingBox : public Object
{
public:
    virtual ~BoundingBox();
    BoundingBox(const glm::vec3& mMin = glm::vec3(0.0f),
                const glm::vec3& mMax = glm::vec3(0.0f));

    void initialize();
    void deinitialize();

    const glm::vec3& getMin() const;
    void setMin(const glm::vec3& pMin);

    const glm::vec3& getMax() const;
    void setMax(const glm::vec3& pMax);

private:
    glm::vec3 mMin;
    glm::vec3 mMax;
};

}

#endif
