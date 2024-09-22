#ifndef LAKOT_LOADER_H
#define LAKOT_LOADER_H

#include <lakot/abstract/resource/loader/ALoader.h>

namespace lakot {

class Loader : virtual public ALoader
{
public:
    virtual ~Loader();
    Loader();
};

}

#endif
