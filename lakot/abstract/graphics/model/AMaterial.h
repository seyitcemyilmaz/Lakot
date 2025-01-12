#ifndef LAKOT_AMATERIAL_H
#define LAKOT_AMATERIAL_H

#include <lakot/utilities/Object.h>

namespace lakot {

class AMaterial : public Object
{
public:
    virtual ~AMaterial() override;
    AMaterial();
};

}

#endif
