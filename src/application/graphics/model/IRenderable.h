#ifndef LAKOT_IRENDERABLE_H
#define LAKOT_IRENDERABLE_H

#include "core/helper/shader/IShader.h"

class IRenderable {
public:
	virtual void draw(IShader* pShader) = 0;
};

#endif
