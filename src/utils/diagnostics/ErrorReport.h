#ifndef LAKOT_ERRORREPORT_H
#define LAKOT_ERRORREPORT_H

#include <exception>
#include <source_location>
#include <string_view>

namespace lakot {

class ErrorReport
{
public:
    static void report(std::string_view pContext, const std::exception& pException,
                       std::source_location pLocation = std::source_location::current());

    static void reportUnknown(std::string_view pContext, std::source_location pLocation = std::source_location::current());

private:
    ErrorReport() = delete;
    ~ErrorReport() = delete;
};

}

#endif
