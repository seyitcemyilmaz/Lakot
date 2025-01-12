#ifndef LAKOT_ARENDERER_H
#define LAKOT_ARENDERER_H

#include "ARenderable.h"

namespace lakot {

class ARenderer : public Object
{
public:
    virtual ~ARenderer() override;
    ARenderer();

    virtual void render(ARenderable* pRenderable) = 0;

    void setProjectionMatrix(const glm::mat4& pProjectionMatrix);
    void setViewMatrix(const glm::mat4& pViewMatrix);

protected:
    glm::mat4 mProjectionMatrix;
    glm::mat4 mViewMatrix;
};

}

#endif
