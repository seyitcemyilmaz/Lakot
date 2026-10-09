#ifndef LAKOT_SERVER_SCRIPTENGINE_H
#define LAKOT_SERVER_SCRIPTENGINE_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>

namespace lakot
{

class Character;

// A character as scripts see it.
//
// Scripts get this rather than a raw Character* for one reason: lifetime. A
// script can stash whatever it is handed in a global and use it minutes
// later, by which point the character may have logged out and the Character
// object is gone. Holding the target behind a pointer that the engine NULLS
// when the call ends turns that from a use-after-free into an ordinary Lua
// error, which pcall catches - which is the whole reason scripting was chosen
// over C++ in the first place.
class ScriptCharacter
{
public:
    Character* target = nullptr;

    // Every binding goes through this, so exactly one place decides what an
    // expired handle does.
    Character& require() const;
};

// One Lua state and the game API bound into it.
//
// Owned per Zone, and therefore per thread: Lua states are completely
// independent of each other, which is precisely why Lua was chosen over
// Python here - two zones run scripts genuinely in parallel with no shared
// interpreter and no lock. Nothing in this class is thread safe, and nothing
// needs to be.
class ScriptEngine
{
public:
    ScriptEngine();

    // Loads (or re-loads) every .lua file in pDirectory. Returns false if the
    // directory is missing or a script failed to compile; a broken script is
    // reported and skipped rather than aborting the rest.
    bool loadDirectory(const std::string& pDirectory);

    // Fires the script hook of the same name, if any script defined one.
    // A missing hook is not an error - most content will define only some.
    void onEnterWorld(Character& pCharacter);
    void onMonsterKilled(Character& pCharacter, uint32_t pTemplateId);
    void onLevelUp(Character& pCharacter, uint32_t pNewLevel);

private:
    sol::state mLua;

    // Reused across calls, but its target is cleared after every one - see
    // ScriptCharacter.
    std::shared_ptr<ScriptCharacter> mCharacterHandle;

    // Event name -> every handler registered for it, in load order.
    //
    // Handlers are registered with lakot.on(), NOT by defining a global
    // function of a well-known name. That was the first design and it is
    // quietly broken: two content files both defining on_enter_world means
    // the second one loaded silently REPLACES the first, so only one script
    // in the whole game can ever react to a given event. Found by a test
    // where a deliberately broken script wiped out a working one.
    std::unordered_map<std::string, std::vector<sol::protected_function>> mHandlers;

    void bindApi();

    // Runs every handler registered for pName, each in its own pcall, and
    // keeps going after one fails. Per-handler isolation matters as much as
    // per-script: one broken quest must not stop the others from running,
    // which is the entire reason this logic lives in script and not C++.
    template <typename... TArgs>
    void callCharacterHook(const char* pName, Character& pCharacter, TArgs... pArgs);
};

}

#endif
