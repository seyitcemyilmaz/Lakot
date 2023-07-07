#include "PlatformFactory.h"

#include "helper/WindowManager.h"

#include "WindowsPlatform.h"

PlatformFactory::PlatformFactory() { }

Platform* PlatformFactory::createPlatformObject() {
#ifdef LAKOT_PLATFORM_WINDOWS
	return createWindowsPlatformObject();
#elif
#error "Not implemented."
#endif
}

Platform* PlatformFactory::createWindowsPlatformObject() {
	Platform* tPlatform = new WindowsPlatform();

	tPlatform->getGraphicsAPI()->initializeGraphicsAPI();

	if (!tPlatform->getGraphicsAPI()->getIsGraphicsAPIInitialized()) {
		throw "Graphics API is not initialized.";
	}

	std::cout << "Lakot Graphics API: " << tPlatform->getGraphicsAPI()->getGraphicsAPIString() << std::endl;

	WindowManager::getInstance()->initializateWindow();

	return tPlatform;
}
