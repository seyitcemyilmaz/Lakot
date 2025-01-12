#ifndef LAKOT_ENGINE_H
#define LAKOT_ENGINE_H

#include <lakot/utilities/Object.h>

namespace lakot {

class Engine : public Object
{
public:
    virtual ~Engine() override;
    Engine();

    static Engine* getInstance();

    void initialize() override;
    void deinitialize() override;

    void run();

protected:
    static Engine* mEngine;

private:
    void initializeFileManager();
    void initializeGarbageCollector();
    void initializeGraphicsAPI();
    void initializeWindow();
    void initializeLayer();
    void initializeRenderer();

    void test();
};

}

#endif
