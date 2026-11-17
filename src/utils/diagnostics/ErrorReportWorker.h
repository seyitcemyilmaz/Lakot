#ifndef LAKOT_ERRORREPORTWORKER_H
#define LAKOT_ERRORREPORTWORKER_H

#include <functional>

namespace lakot {

class ErrorReportWorker
{
public:
    using Job = std::function<void()>;

    static bool post(Job pJob);

private:
    ErrorReportWorker() = delete;
    ~ErrorReportWorker() = delete;
};

}

#endif
