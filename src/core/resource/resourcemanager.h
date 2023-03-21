#ifndef LAKOT_RESOURCEMANAGER_H
#define LAKOT_RESOURCEMANAGER_H

#include <iostream>

#include "modelloader.h"

class ResourceManager {
private:
    static ResourceManager* mInstance;

    std::map<std::string, ModelResource*> mModelResources;

    ResourceManager();
public:
    static ResourceManager* getInstance();

    void addModelResource(std::string pModelFile);
};

#endif
