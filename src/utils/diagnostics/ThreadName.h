#ifndef LAKOT_THREADNAME_H
#define LAKOT_THREADNAME_H

#include <string_view>

namespace lakot {

class ThreadName
{
public:
    static void set(std::string_view pName);
    static const char* get();

private:
    ThreadName() = delete;
    ~ThreadName() = delete;
};

}

#endif
