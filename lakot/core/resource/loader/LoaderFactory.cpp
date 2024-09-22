#include "LoaderFactory.h"

#include <spdlog/spdlog.h>

#include "ModelLoader.h"
#include "NodeLoader.h"
#include "MeshLoader.h"

using namespace lakot;

LoaderFactory::~LoaderFactory()
{

}

LoaderFactory::LoaderFactory()
{

}

Loader* LoaderFactory::createLoader(LoaderType pLoaderType)
{
    if (pLoaderType == LoaderType::eNone)
    {
        spdlog::error("Loader type is none.");
        return nullptr;
    }

    if (pLoaderType == LoaderType::eModel)
    {
        return new ModelLoader();
    }

    if (pLoaderType == LoaderType::eNode)
    {
        return new NodeLoader();
    }

    if (pLoaderType == LoaderType::eMesh)
    {
        return new MeshLoader();
    }

    throw "Undefined Loader Type.";
}
