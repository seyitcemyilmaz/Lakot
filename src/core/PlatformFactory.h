#ifndef PLATFORMFACTORY_H
#define PLATFORMFACTORY_H

#include "platform.h"

class PlatformFactory
{
private:
	PlatformFactory();

	static Platform* createWindowsPlatformObject();
public:
	static Platform* createPlatformObject();
};

#endif // PLATFORMFACTORY_H
