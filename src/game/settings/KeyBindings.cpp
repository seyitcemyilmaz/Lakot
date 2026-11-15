#include "KeyBindings.h"

#include <fstream>

using namespace lakot;

namespace
{
    constexpr size_t kActionCount = static_cast<size_t>(KeyActionType::eCount);

    struct ActionInfo
    {
        const char* id;
        SDL_Scancode defaultKey;
    };

    const ActionInfo kActions[kActionCount] =
    {
        { "move_forward", SDL_SCANCODE_W },
        { "move_backward", SDL_SCANCODE_S },
        { "move_left", SDL_SCANCODE_A },
        { "move_right", SDL_SCANCODE_D },
        { "attack", SDL_SCANCODE_SPACE },
        { "auto_pickup", SDL_SCANCODE_Z },
        { "inventory", SDL_SCANCODE_I },
        { "world_map", SDL_SCANCODE_M },
        { "character_sheet", SDL_SCANCODE_C },
        { "skill_window", SDL_SCANCODE_K },
        { "skill_slot_1", SDL_SCANCODE_1 },
        { "skill_slot_2", SDL_SCANCODE_2 },
        { "skill_slot_3", SDL_SCANCODE_3 },
        { "skill_slot_4", SDL_SCANCODE_4 },
        { "skill_slot_5", SDL_SCANCODE_5 },
        { "skill_slot_f1", SDL_SCANCODE_F1 },
        { "skill_slot_f2", SDL_SCANCODE_F2 },
        { "skill_slot_f3", SDL_SCANCODE_F3 },
        { "skill_slot_f4", SDL_SCANCODE_F4 },
        { "skill_slot_f5", SDL_SCANCODE_F5 }
    };
}

KeyBindings::KeyBindings()
{
    resetToDefaults();
}

void KeyBindings::load()
{
    resetToDefaults();

    std::array<SDL_Scancode, kActionCount> tLoaded = mKeys;
    std::array<bool, kActionCount> tIsLoaded{};

    auto tTrim = [](const std::string& pText)
    {
        const char* kSpaces = " \t\r\n";
        size_t tBegin = pText.find_first_not_of(kSpaces);

        if (tBegin == std::string::npos)
        {
            return std::string();
        }

        return pText.substr(tBegin, pText.find_last_not_of(kSpaces) - tBegin + 1);
    };

    std::ifstream tFile(getFilePath());
    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        size_t tSeparator = tLine.find('=');

        if (tSeparator == std::string::npos)
        {
            continue;
        }

        std::string tId = tTrim(tLine.substr(0, tSeparator));
        std::string tValue = tTrim(tLine.substr(tSeparator + 1));

        for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
        {
            if (tId != kActions[tIndex].id)
            {
                continue;
            }

            SDL_Scancode tKey = tValue == "none" ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromName(tValue.c_str());

            if (tValue == "none" || (tKey != SDL_SCANCODE_UNKNOWN && !isReservedKey(tKey)))
            {
                tLoaded[tIndex] = tKey;
                tIsLoaded[tIndex] = true;
            }

            break;
        }
    }

    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        if (tIsLoaded[tIndex])
        {
            bool tIsTaken = false;

            for (size_t tOther = 0; tOther < tIndex; ++tOther)
            {
                tIsTaken = tIsTaken || (tIsLoaded[tOther] && tLoaded[tOther] != SDL_SCANCODE_UNKNOWN && tLoaded[tOther] == tLoaded[tIndex]);
            }

            mKeys[tIndex] = tIsTaken ? SDL_SCANCODE_UNKNOWN : tLoaded[tIndex];
        }
    }

    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        if (tIsLoaded[tIndex])
        {
            continue;
        }

        for (size_t tOther = 0; tOther < kActionCount; ++tOther)
        {
            if (tOther != tIndex && tIsLoaded[tOther] && mKeys[tOther] == mKeys[tIndex])
            {
                mKeys[tIndex] = SDL_SCANCODE_UNKNOWN;
                break;
            }
        }
    }
}

void KeyBindings::save() const
{
    std::ofstream tFile(getFilePath(), std::ios::trunc);

    if (!tFile)
    {
        SDL_Log("Key bindings could not be saved to %s", getFilePath().c_str());
        return;
    }

    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        tFile << kActions[tIndex].id << "=" << (mKeys[tIndex] == SDL_SCANCODE_UNKNOWN ? "none" : SDL_GetScancodeName(mKeys[tIndex])) << "\n";
    }
}

SDL_Scancode KeyBindings::getKey(KeyActionType pAction) const
{
    return mKeys[static_cast<size_t>(pAction)];
}

std::optional<KeyActionType> KeyBindings::findAction(SDL_Scancode pKey) const
{
    if (pKey == SDL_SCANCODE_UNKNOWN)
    {
        return std::nullopt;
    }

    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        if (mKeys[tIndex] == pKey)
        {
            return static_cast<KeyActionType>(tIndex);
        }
    }

    return std::nullopt;
}

bool KeyBindings::rebind(KeyActionType pAction, SDL_Scancode pKey)
{
    if (pKey == SDL_SCANCODE_UNKNOWN || isReservedKey(pKey))
    {
        return false;
    }

    for (SDL_Scancode& tKey : mKeys)
    {
        if (tKey == pKey)
        {
            tKey = SDL_SCANCODE_UNKNOWN;
        }
    }

    mKeys[static_cast<size_t>(pAction)] = pKey;
    save();
    return true;
}

void KeyBindings::resetToDefaults()
{
    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        mKeys[tIndex] = kActions[tIndex].defaultKey;
    }
}

const char* KeyBindings::getActionId(KeyActionType pAction)
{
    return kActions[static_cast<size_t>(pAction)].id;
}

std::string KeyBindings::getKeyLabel(SDL_Scancode pKey)
{
    if (pKey == SDL_SCANCODE_UNKNOWN)
    {
        return "-";
    }

    return SDL_GetScancodeName(pKey);
}

bool KeyBindings::isReservedKey(SDL_Scancode pKey)
{
    switch (pKey)
    {
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_F6:
        case SDL_SCANCODE_F9:
        case SDL_SCANCODE_F10:
            return true;
        default:
            return false;
    }
}

std::string KeyBindings::getFilePath()
{
    std::string tPath;

    if (char* tPrefPath = SDL_GetPrefPath("Lakot", "LakotGame"))
    {
        tPath = tPrefPath;
        SDL_free(tPrefPath);
    }

    return tPath + "keybindings.cfg";
}
