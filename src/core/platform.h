#ifndef LAKOT_PLATFORM_H
#define LAKOT_PLATFORM_H

#define LAKOT_PLATFORM_NONE                     0
#define LAKOT_PLATFORM_WINDOWS                  1
#define LAKOT_PLATFORM_LINUX                    2
#define LAKOT_PLATFORM_ANDROID                  3

#define LAKOT_GRAPHICS_API_NONE                 0
#define LAKOT_GRAPHICS_API_OPENGL               1
#define LAKOT_GRAPHICS_API_OPENGLES             2
//#define LAKOT_GRAPHICS_API_VULKAN               3
//#define LAKOT_GRAPHICS_API_D3DX                 4

#if defined(WIN32) || defined(_WIN32)
    #define LAKOT_PLATFORM                      LAKOT_PLATFORM_WINDOWS
    #define LAKOT_GRAPHICS_API                  LAKOT_GRAPHICS_API_OPENGL
    #define LAKOT_GRAPHICS_API_VERSION_MAJOR    3
    #define LAKOT_GRAPHICS_API_VERSION_MINOR    3

    #define ContextWindow                       GLFWwindow

    #define LAKOT_SHADER_HEADER                 "#version 330 core\n"
    
    #include "graphicsAPI/openglapi.h"
    #include <iostream>
    #include <filesystem>
    #include <fstream>
    #include <vector>
    #include <map>
    #include <algorithm>

#elif defined(UNIX) || defined(__unix__) || defined(LINUX) || defined(__linux__)
    #define LAKOT_PLATFORM                      LAKOT_PLATFORM_LINUX
    #define LAKOT_GRAPHICS_API                  LAKOT_GRAPHICS_API_OPENGL
    #define LAKOT_GRAPHICS_API_VERSION_MAJOR    3
    #define LAKOT_GRAPHICS_API_VERSION_MINOR    3

    #define ContextWindow                       GLFWwindow

    #include "graphicsAPI/openglapi.h"

#elif defined(ANDROID) || defined(__ANDROID__)
    #define LAKOT_PLATFORM                      LAKOT_PLATFORM_ANDROID
    #define LAKOT_GRAPHICS_API                  LAKOT_GRAPHICS_API_OPENGLES
    #define LAKOT_GRAPHICS_API_VERSION_MAJOR    3
    #define LAKOT_GRAPHICS_API_VERSION_MINOR    2
#else
    #define LAKOT_PLATFORM                      LAKOT_PLATFORM_NONE
    #define LAKOT_GRAPHICS_API                  LAKOT_GRAPHICS_API_NONE
    #define LAKOT_GRAPHICS_API_VERSION_MAJOR    0
    #define LAKOT_GRAPHICS_API_VERSION_MINOR    0
    #error "Platform is not detected."
#endif

#ifndef ContextWindow
    #error ContextWindow is not defined.
#endif

class Platform {
private:
    GraphicsAPI* mGraphicsAPI;
public:
    Platform();
    ~Platform();

    GraphicsAPI* getGraphicsAPI();

    static std::string getLakotRootPath();
    static std::string getLakotAssetsPath();
    static std::string getLakotModelsPath();
    static std::string readFileContent(std::string pFilePath);
};


#endif
