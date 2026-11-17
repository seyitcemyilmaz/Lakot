#ifndef LAKOT_CRASHHANDLER_H
#define LAKOT_CRASHHANDLER_H

#include <string_view>

namespace lakot {

class CrashHandler
{
public:
    static void install(std::string_view pExecutableName);

    static void attachThread(std::string_view pThreadName);

    static void recordLog(std::string_view pText);

    static const char* getExecutableName();

private:
    CrashHandler() = delete;
    ~CrashHandler() = delete;
};

}

#endif
