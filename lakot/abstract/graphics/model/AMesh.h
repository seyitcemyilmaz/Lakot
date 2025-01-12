#ifndef LAKOT_AMESH_H
#define LAKOT_AMESH_H

#include <string>

#include <lakot/utilities/Object.h>
#include <lakot/utilities/BoundingBox.h>

namespace lakot {

class AMesh : public Object
{
public:
    virtual ~AMesh() override;
    AMesh(const BoundingBox& pBoundingBox);

    virtual const std::string& getName() const = 0;

    BoundingBox& getBoundingBox();

protected:
    BoundingBox mBoundingBox;
};

}

#endif
