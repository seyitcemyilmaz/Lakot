#ifndef WINDOWSPLATFORM_H
#define WINDOWSPLATFORM_H

#include "Platform.h"

class WindowsPlatform : public Platform {
private:
	void processKeyboardInputs(double pDt);
	void processMouseInputs();

public:
	WindowsPlatform();

	std::string getRootPath() override;

	void processInputs() override;
	void run(const std::function<void()>& pRenderFunction) override;
};

#endif // WINDOWSPLATFORM_H
