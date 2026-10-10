#include "SkillBarBindings.h"

#include <cstdio>
#include <fstream>

#include <SDL3/SDL.h>

using namespace lakot;

SkillBarBindings::SkillBarBindings(uint64_t pCharacterId)
    : mCharacterId(pCharacterId)
{
    load();
}

std::optional<uint32_t> SkillBarBindings::getTemplateId(size_t pSlot) const
{
    if (pSlot >= kSlotCount || mTemplateIds[pSlot] == 0)
    {
        return std::nullopt;
    }

    return mTemplateIds[pSlot];
}

void SkillBarBindings::setTemplateId(size_t pSlot, uint32_t pTemplateId)
{
    if (pSlot >= kSlotCount || mTemplateIds[pSlot] == pTemplateId)
    {
        return;
    }

    mTemplateIds[pSlot] = pTemplateId;
    save();
}

void SkillBarBindings::clear(size_t pSlot)
{
    setTemplateId(pSlot, 0);
}

void SkillBarBindings::load()
{
    std::ifstream tFile(getFilePath());
    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        unsigned int tSlot = 0;
        unsigned int tTemplateId = 0;

        if (std::sscanf(tLine.c_str(), "slot%u=%u", &tSlot, &tTemplateId) == 2 && tSlot < kSlotCount)
        {
            mTemplateIds[tSlot] = tTemplateId;
        }
    }
}

void SkillBarBindings::save() const
{
    std::ofstream tFile(getFilePath(), std::ios::trunc);

    if (!tFile)
    {
        SDL_Log("Skill bar could not be saved to %s", getFilePath().c_str());
        return;
    }

    for (size_t tSlot = 0; tSlot < kSlotCount; ++tSlot)
    {
        if (mTemplateIds[tSlot] != 0)
        {
            tFile << "slot" << tSlot << "=" << mTemplateIds[tSlot] << '\n';
        }
    }
}

std::string SkillBarBindings::getFilePath() const
{
    std::string tPath;

    if (char* tPrefPath = SDL_GetPrefPath("Lakot", "LakotGame"))
    {
        tPath = tPrefPath;
        SDL_free(tPrefPath);
    }

    return tPath + "skillbar-" + std::to_string(mCharacterId) + ".cfg";
}
