#include "Logger.h"

#include <spdlog/spdlog.h>

#if defined(LAKOT_PLATFORM_ANDROID)
#include <spdlog/sinks/android_sink.h>
#endif

using namespace lakot;

Logger* Logger::mInstance = nullptr;

Logger::~Logger()
{

}

Logger::Logger()
    : ALogger()
{

}

Logger* Logger::getInstance()
{
    if (!mInstance)
    {
        mInstance = new Logger();
    }

    return mInstance;
}

void Logger::initialize()
{    
#if defined(LAKOT_PLATFORM_ANDROID)
    std::string tTag = "lakot";
    auto tLogger = spdlog::android_logger_mt("lakot", tTag);
    spdlog::set_default_logger(tLogger);
#endif
}

void Logger::deinitialize()
{

}
