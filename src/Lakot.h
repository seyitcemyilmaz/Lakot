#ifndef LAKOT_LAKOT_H
#define LAKOT_LAKOT_H

#include "Application.h"

class Lakot {
private:
	Application* mApplication;

public:
	Lakot();
	~Lakot();

	void initalizeApplication();
	void runApplication();
	void terminateApplication();
};

#endif
