#include "platform.h"

Platform::Platform() {
    #if LAKOT_PLATFORM == LAKOT_PLATFORM_WINDOWS
        mGraphicsAPI = new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_LINUX
        mGraphicsAPI = new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_ANDROID
        throw "Not implemented.";
    #else
        throw "Unknown platform.";
    #endif
}

GraphicsAPI* Platform::getGraphicsAPI() {
    return mGraphicsAPI;
}

std::string Platform::getLakotRootPath() {
    #if LAKOT_PLATFORM == LAKOT_PLATFORM_WINDOWS
        std::filesystem::path tPlatformFilePath = __FILE__;
        std::filesystem::path tCorePath = tPlatformFilePath.parent_path();
        std::filesystem::path tSrcPath = tCorePath.parent_path();
        std::filesystem::path tLakotRootPath = tSrcPath.parent_path();
        return tLakotRootPath.string();
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_LINUX
        throw "Not implemented.";
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_ANDROID
        throw "Not implemented.";
    #else
        throw "Unknown platform.";
    #endif
}

std::string Platform::getLakotAssetsPath() {
    std::string tLakotAssetsPath = getLakotRootPath() + "/assets";
    return tLakotAssetsPath;
}

std::string Platform::getLakotModelsPath() {
    std::string tLakotModelsPath = getLakotAssetsPath() + "/models";
    return tLakotModelsPath;
}

std::string Platform::readFileContent(std::string pFilePath) {
    #if LAKOT_PLATFORM == LAKOT_PLATFORM_WINDOWS
        std::ifstream tFile(pFilePath);

        if (!tFile.is_open()) {
            throw "File is not found.";
        }

        std::string tFileContent;
        std::getline(tFile, tFileContent, '\0');

        return tFileContent;
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_LINUX
        throw "Not implemented.";
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_ANDROID
        throw "Not implemented.";
    #else
        throw "Unknown platform.";
    #endif
}

Platform::~Platform() {
    delete mGraphicsAPI;
}
