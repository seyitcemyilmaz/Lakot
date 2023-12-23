#include "Settings.h"

Settings* Settings::mInstance = nullptr;

Settings* Settings::getInstance() {
	if (!mInstance) {
		mInstance = new Settings();
	}

	return mInstance;
}

Settings::Settings()
	: mIsMouseMovementEnabled(false)
{

}

bool Settings::getIsMouseMovementEnabled()
{
	return mIsMouseMovementEnabled;
}

void Settings::setIsMouseMovementEnabled(bool pIsMouseMovementEnabled)
{
	mIsMouseMovementEnabled = pIsMouseMovementEnabled;
}


