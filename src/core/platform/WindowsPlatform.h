#ifndef WINDOWSPLATFORM_H
#define WINDOWSPLATFORM_H

#include "Platform.h"

class WindowsPlatform : public Platform {
public:
	WindowsPlatform();

	std::string getRootPath() override;

	void processInputs() override;
	void run() override;
};

#endif // WINDOWSPLATFORM_H
