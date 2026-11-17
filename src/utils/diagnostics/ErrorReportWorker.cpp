#include "diagnostics/ErrorReportWorker.h"

#include "diagnostics/CrashHandler.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include <windows.h>

using namespace lakot;

namespace
{
    constexpr std::size_t kMaxPendingJobs = 16;

    struct WorkerState
    {
        std::mutex mutex;
        std::condition_variable condition;
        std::deque<ErrorReportWorker::Job> jobs;
        bool isStarted = false;
    };

    // Leaked on purpose: the detached worker may still run during static destruction.
    WorkerState& state()
    {
        static WorkerState* sState = new WorkerState;
        return *sState;
    }

    void run(WorkerState& pState)
    {
        CrashHandler::attachThread("error-report");
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        while (true)
        {
            ErrorReportWorker::Job tJob;

            {
                std::unique_lock<std::mutex> tLock(pState.mutex);
                pState.condition.wait(tLock, [&pState]() { return !pState.jobs.empty(); });
                tJob = std::move(pState.jobs.front());
                pState.jobs.pop_front();
            }

            tJob();
        }
    }
}

bool ErrorReportWorker::post(Job pJob)
{
    WorkerState& tState = state();

    {
        std::lock_guard<std::mutex> tLock(tState.mutex);

        if (tState.jobs.size() >= kMaxPendingJobs)
        {
            return false;
        }

        tState.jobs.push_back(std::move(pJob));

        if (!tState.isStarted)
        {
            tState.isStarted = true;
            std::thread([&tState]() { run(tState); }).detach();
        }
    }

    tState.condition.notify_one();
    return true;
}
