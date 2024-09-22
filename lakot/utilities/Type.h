#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

#include <string>

namespace lakot {

enum class AssetType
{
    eNone,
    eModel,
    eNode,
    eMesh
};

enum class EntityType
{
    eNone,
    eModel,
    eNode,
    eMesh
};

enum class LoaderType
{
    eNone,
    eModel,
    eNode,
    eMesh
};

enum class ButtonType
{
    eUndefined,
    eLeft,
    eRight,
    eMiddle
};

}

#endif
