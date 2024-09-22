#ifndef LAKOT_ALOADER_H
#define LAKOT_ALOADER_H

#include <lakot/interface/resource/loader/ILoader.h>

namespace lakot {

class ALoader : virtual public ILoader
{
public:
    virtual ~ALoader() override;
    ALoader();

    LoaderType getType() const override;

protected:
    LoaderType mType;
};

}

#endif
