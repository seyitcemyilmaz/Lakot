#include "AGarbageCollector.h"

using namespace lakot;

AGarbageCollector::~AGarbageCollector()
{

}

AGarbageCollector::AGarbageCollector()
    : IGarbageCollector()
{

}

void AGarbageCollector::add(IObject* pObject)
{
    const std::lock_guard<std::mutex> tLock(mMutex);
    mObjectQueue.push(pObject);
}
