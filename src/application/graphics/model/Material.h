#ifndef LAKOT_MATERIAL_H
#define LAKOT_MATERIAL_H

#include "resource/MaterialResource.h"

class Material
{
private:
    MaterialResource* mMaterialResource;

public:
    Material(MaterialResource* pMaterialResource);

    MaterialResource* getMaterialResource();
};

#endif
