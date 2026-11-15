#ifndef LAKOT_KEY_BINDINGS_H
#define LAKOT_KEY_BINDINGS_H

#include <array>
#include <optional>
#include <string>

#include <SDL3/SDL.h>

namespace lakot
{

enum class KeyActionType
{
    eMoveForward,
    eMoveBackward,
    eMoveLeft,
    eMoveRight,
    eAttack,
    eAutoPickup,
    eInventory,
    eWorldMap,
    eCharacterSheet,
    eSkillWindow,
    eSkillSlot1,
    eSkillSlot2,
    eSkillSlot3,
    eSkillSlot4,
    eSkillSlot5,
    eSkillSlotF1,
    eSkillSlotF2,
    eSkillSlotF3,
    eSkillSlotF4,
    eSkillSlotF5,
    eCount
};

class KeyBindings
{
public:
    KeyBindings();

    void load();
    void save() const;

    SDL_Scancode getKey(KeyActionType pAction) const;
    std::optional<KeyActionType> findAction(SDL_Scancode pKey) const;

    bool rebind(KeyActionType pAction, SDL_Scancode pKey);
    void resetToDefaults();

    static const char* getActionId(KeyActionType pAction);
    static std::string getKeyLabel(SDL_Scancode pKey);
    static bool isReservedKey(SDL_Scancode pKey);

private:
    std::array<SDL_Scancode, static_cast<size_t>(KeyActionType::eCount)> mKeys;

    static std::string getFilePath();
};

}

#endif
