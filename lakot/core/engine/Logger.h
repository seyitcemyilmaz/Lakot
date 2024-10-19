#ifndef LAKOT_LOGGER_H
#define LAKOT_LOGGER_H

#include <lakot/utilities/Type.h>
#include <lakot/abstract/engine/ALogger.h>

namespace lakot {

class Logger : public ALogger
{
public:
    static Logger* getInstance();

    void initialize() override;
    void deinitialize() override;

    // template <typename T>
    // inline void setVariable(const Variable<T>& pVariable)
    // {
    //     setVariable(std::make_shared<Variable<T>>(pVariable), typeid(T));
    // }

    // void setVariable(const std::shared_ptr<void>& pVariable, const std::type_info& type) override;

protected:
    virtual ~Logger() override;
    Logger();

    static Logger* mInstance;
};

}

#endif
