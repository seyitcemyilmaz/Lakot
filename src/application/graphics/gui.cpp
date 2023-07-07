#include "GUI.h"

GUI::GUI() {
	mVisibility = false;
}

void GUI::initialize() { }

void GUI::setVisibility(bool pVisibility) {
	mVisibility = pVisibility;
}

bool GUI::getVisibility() {
	return mVisibility;
}
