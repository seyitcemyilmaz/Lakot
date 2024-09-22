#ifndef LAKOT_LOADERFACTORY_H
#define LAKOT_LOADERFACTORY_H

#include "Loader.h"

namespace lakot {

class LoaderFactory
{
public:
    static Loader* createLoader(LoaderType pLoaderType);

private:
    virtual ~LoaderFactory();
    LoaderFactory();
};

}

#endif
