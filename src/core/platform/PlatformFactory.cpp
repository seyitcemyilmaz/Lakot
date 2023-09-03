#include "PlatformFactory.h"

#include "helper/window/WindowManager.h"

#include "WindowsPlatform.h"

PlatformFactory::PlatformFactory() = default;

Platform* PlatformFactory::createPlatformObject() {
#ifdef LAKOT_PLATFORM_WINDOWS
	return createWindowsPlatformObject();
#else
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
