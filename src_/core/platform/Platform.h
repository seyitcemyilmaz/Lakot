#ifndef LAKOT_PLATFORM_H
#define LAKOT_PLATFORM_H

#define LAKOT_PLATFORM_NONE						0
#define LAKOT_PLATFORM_WINDOWS					1
#define LAKOT_PLATFORM_LINUX					2
#define LAKOT_PLATFORM_ANDROID					3

#define LAKOT_GRAPHICS_API_NONE					0
#define LAKOT_GRAPHICS_API_OPENGL				1
#define LAKOT_GRAPHICS_API_OPENGLES				2
//#define LAKOT_GRAPHICS_API_VULKAN				3
//#define LAKOT_GRAPHICS_API_D3DX				4

#if defined(WIN32) || defined(_WIN32)
#define LAKOT_PLATFORM							LAKOT_PLATFORM_WINDOWS
#define LAKOT_GRAPHICS_API						LAKOT_GRAPHICS_API_OPENGL
#define LAKOT_GRAPHICS_API_VERSION_MAJOR		3
#define LAKOT_GRAPHICS_API_VERSION_MINOR		3

#define LAKOT_SHADER_HEADER						"#version 330 core\n"

#define LAKOT_FILE_PATH_SEPARATOR				"\\"

#include "graphicsAPI/OpenGLAPI.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#elif defined(UNIX) || defined(__unix__) || defined(LINUX) || defined(__linux__)
#define LAKOT_PLATFORM							LAKOT_PLATFORM_LINUX
#define LAKOT_GRAPHICS_API						LAKOT_GRAPHICS_API_OPENGL
#define LAKOT_GRAPHICS_API_VERSION_MAJOR		3
#define LAKOT_GRAPHICS_API_VERSION_MINOR		3

#define LAKOT_FILE_PATH_SEPARATOR				"/"

#include "graphicsAPI/openglapi.h"

#elif defined(ANDROID) || defined(__ANDROID__)
#define LAKOT_PLATFORM							LAKOT_PLATFORM_ANDROID
#define LAKOT_GRAPHICS_API						LAKOT_GRAPHICS_API_OPENGLES
#define LAKOT_GRAPHICS_API_VERSION_MAJOR		3
#define LAKOT_GRAPHICS_API_VERSION_MINOR		2

#define LAKOT_FILE_PATH_SEPARATOR				"/"

#else
#define LAKOT_PLATFORM							LAKOT_PLATFORM_NONE
#define LAKOT_GRAPHICS_API						LAKOT_GRAPHICS_API_NONE
#define LAKOT_GRAPHICS_API_VERSION_MAJOR		0
#define LAKOT_GRAPHICS_API_VERSION_MINOR		0

#define LAKOT_FILE_PATH_SEPARATOR				"/"

#error "Platform is not detected."
#endif

class Platform {
private:
	GraphicsAPI* mGraphicsAPI;

protected:
	double mCurrentTime;
	double mPreviousTime;

public:
	Platform(GraphicsAPI* pGraphicsAPI);
	virtual ~Platform();

	GraphicsAPI* getGraphicsAPI();

	virtual std::string getRootPath() = 0;
	std::string getAssetsPath();
	std::string getModelsPath();

	virtual void processInputs() = 0;
	virtual void run(std::function<void()> pRenderFunction) = 0;
};


#endif
