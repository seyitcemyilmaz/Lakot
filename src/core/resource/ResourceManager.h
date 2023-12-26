#ifndef LAKOT_RESOURCEMANAGER_H
#define LAKOT_RESOURCEMANAGER_H

#include "ModelResource.h"

class ResourceManager
{
private:
    static ResourceManager* mInstance;

    std::map<std::string, ModelResource*> mModelResources;

    ResourceManager();

public:
    static ResourceManager* getInstance();

    void addModelResource(const std::string& pModelFilePath);

    ModelResource* getModelResource(const std::string& pModelFilePath);
};

#endif
