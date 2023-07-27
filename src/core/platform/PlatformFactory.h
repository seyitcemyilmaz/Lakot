#ifndef PLATFORMFACTORY_H
#define PLATFORMFACTORY_H

#include "Platform.h"

class PlatformFactory {
private:
	PlatformFactory();

	static Platform* createWindowsPlatformObject();
public:
	static Platform* createPlatformObject();
};

#endif // PLATFORMFACTORY_H
