#ifndef LAKOT_ALAYER_H
#define LAKOT_ALAYER_H

#include <lakot/utilities/Object.h>

namespace lakot {

class AScene;

class ALayer : public Object
{
public:
    virtual ~ALayer() override;
    ALayer();

    virtual void update() = 0;

    AScene* getScene() const;

protected:
    AScene* mScene;
};

}

#endif
