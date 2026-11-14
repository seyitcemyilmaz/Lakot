#ifndef LAKOT_EFFECT_CATALOG_H
#define LAKOT_EFFECT_CATALOG_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "EffectDefinition.h"

namespace lakot
{

class EffectCatalog
{
public:
    bool load(const std::string& pPath, std::string& pError);

    const EffectDefinition* find(std::string_view pName) const;

    const std::vector<EffectDefinition>& getEffects() const;
    const std::vector<std::string>& getTextureNames() const;

private:
    std::vector<std::string> mTextureNames;
    std::vector<EffectDefinition> mEffects;
    std::map<std::string, size_t, std::less<>> mIndexByName;
};

}

#endif
