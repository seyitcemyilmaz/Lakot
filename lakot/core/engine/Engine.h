#ifndef LAKOT_ENGINE_H
#define LAKOT_ENGINE_H

#include <lakot/abstract/engine/AEngine.h>

namespace lakot {

class Engine : public AEngine
{
public:
    virtual ~Engine() override;
    Engine();

    static Engine* getInstance();

    void initialize() override;
    void deinitialize() override;

    void run() override;

protected:
    static Engine* mEngine;

private:
    void initializeGarbageCollector();
    void initializeGraphicsAPI();
    void initializeWindow();
    void initializeLayer();

    void test();
};

}

#endif
