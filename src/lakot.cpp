#include "lakot.h"

Lakot::Lakot() {
	mApplication = nullptr;
}

void Lakot::initalizeApplication() {
	mApplication = new Application();
	mApplication->initialization();
}

void Lakot::runApplication() {
	std::cout << "Application is running." << std::endl;
	mApplication->run();
	std::cout << "Application is stopped." << std::endl;
}

void Lakot::terminateApplication() {
	mApplication->terminate();
}

Lakot::~Lakot() {
	delete mApplication;
}
