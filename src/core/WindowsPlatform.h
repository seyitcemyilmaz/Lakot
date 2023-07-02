#ifndef WINDOWSPLATFORM_H
#define WINDOWSPLATFORM_H

#include "platform.h"

class WindowsPlatform : public Platform
{
private:
	void processKeyboardInputs(double pDt);
	void processMouseInputs();
public:
	WindowsPlatform();

	std::string getRootPath() override;

	void processInputs(double* pPreviousTime) override;
};

#endif // WINDOWSPLATFORM_H
