#include "GraphicsAPI.h"

using namespace lakot;

GraphicsAPI* GraphicsAPI::mInstance = nullptr;

GraphicsAPI::~GraphicsAPI() {}

GraphicsAPI::GraphicsAPI()
    : AGraphicsAPI()
{

}

GraphicsAPI* GraphicsAPI::getInstance()
{
    return mInstance;
}
