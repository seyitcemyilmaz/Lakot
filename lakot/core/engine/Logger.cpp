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

// void Logger::setVariable(const std::shared_ptr<void>& pVariable, const type_info& type)
// {
//     if (type == typeid(int))
//     {
//         auto var = *std::static_pointer_cast<Variable<int>>(pVariable);
//         spdlog::info("Derived class: Integer Variable: {0}, Value: {1}", var.name, var.value);
//     }
//     else if (type == typeid(std::string))
//     {
//         auto var = *std::static_pointer_cast<Variable<std::string>>(pVariable);
//         spdlog::info("Derived class: String Variable: {0}, Value: {1}", var.name, var.value);
//     }
//     else
//     {
//         IObject::setVariable(pVariable, type);
//     }
// }
