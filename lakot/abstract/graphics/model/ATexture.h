#ifndef LAKOT_ATEXTURE_H
#define LAKOT_ATEXTURE_H

#include <string>

#include <lakot/utilities/Object.h>

namespace lakot {

class ATexture : public Object
{
public:
    virtual ~ATexture() override;
    ATexture();

    virtual const std::string& getPath() const = 0;
};

}

#endif
