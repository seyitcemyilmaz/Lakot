#ifndef LAKOT_ASHADER_H
#define LAKOT_ASHADER_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AShader : public Object
{
public:
    virtual ~AShader() override;
    AShader();

    ShaderType getType() const noexcept;

    unsigned int getId() const;

protected:
    unsigned int mId;

    ShaderType mType;
};

}

#endif
