#include "ScriptEngine.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <syncstream>

#include "../game/Character.h"

using namespace lakot;

Character& ScriptCharacter::require() const
{
    if (!target)
    {
        // Becomes a Lua error, caught by the pcall around the hook - not a
        // crash. This is what a script holding onto a character past the end
        // of the call hits.
        throw sol::error("character handle is no longer valid");
    }

    return *target;
}

ScriptEngine::ScriptEngine()
{
    // Deliberately NOT sol::lib::io, os, package or debug. Content scripts
    // have no business touching the filesystem, spawning processes, loading
    // arbitrary modules, or reaching into the VM - and leaving those out is
    // far easier than trying to take them away afterwards.
    mLua.open_libraries(sol::lib::base,
                        sol::lib::string,
                        sol::lib::table,
                        sol::lib::math);

    mCharacterHandle = std::make_shared<ScriptCharacter>();

    bindApi();
}

void ScriptEngine::bindApi()
{
    // Read-only properties use a getter-only overload on purpose: a script
    // assigning character.level = 99 should fail loudly, not silently do
    // nothing or actually work. Anything that CHANGES a character goes
    // through a named method, so every mutation is explicit at the call site.
    mLua.new_usertype<ScriptCharacter>("Character",
        sol::no_constructor,

        "name", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getName();
        }),
        "level", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getStats().level;
        }),
        "gold", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getStats().gold;
        }),
        "health", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getStats().health;
        }),
        "max_health", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getMaxHealth();
        }),
        "attack", sol::property([](const ScriptCharacter& pSelf)
        {
            return pSelf.require().getAttack();
        }),

        // Returns how many could NOT be given, so a script handing out a
        // reward can react to a full bag instead of silently destroying it.
        "give_item", [](ScriptCharacter& pSelf, uint32_t pTemplateId, uint32_t pCount)
        {
            return pSelf.require().giveItem(pTemplateId, pCount);
        },
        "take_item", [](ScriptCharacter& pSelf, uint32_t pTemplateId, uint32_t pCount)
        {
            return pSelf.require().takeItem(pTemplateId, pCount);
        },
        "count_item", [](const ScriptCharacter& pSelf, uint32_t pTemplateId)
        {
            return pSelf.require().countItem(pTemplateId);
        },
        "add_experience", [](ScriptCharacter& pSelf, uint64_t pAmount)
        {
            return pSelf.require().addExperience(pAmount);
        },
        "add_gold", [](ScriptCharacter& pSelf, int64_t pAmount)
        {
            pSelf.require().addGold(pAmount);
        }
    );

    // Everything the engine offers scripts lives under one table, so it is
    // obvious in a script which calls are ours and which are Lua's.
    sol::table tApi = mLua.create_named_table("lakot");

    tApi.set_function("log", [](const std::string& pMessage)
    {
        std::osyncstream(std::cout) << "[Script] " << pMessage << std::endl;
    });

    // Registration, not a well-known global function name: several content
    // files must be able to react to the same event without any of them
    // knowing the others exist. See mHandlers for what this replaced.
    tApi.set_function("on", [this](const std::string& pEvent, sol::protected_function pHandler)
    {
        if (!pHandler.valid())
        {
            std::osyncstream(std::cerr) << "[Script] lakot.on('" << pEvent << "') needs a function" << std::endl;
            return;
        }

        mHandlers[pEvent].push_back(std::move(pHandler));
    });
}

bool ScriptEngine::loadDirectory(const std::string& pDirectory)
{
    std::error_code tErrorCode;

    if (!std::filesystem::is_directory(pDirectory, tErrorCode))
    {
        std::osyncstream(std::cerr) << "[Script] Klasor bulunamadi: " << pDirectory << std::endl;
        return false;
    }

    // Sorted, so load order is the same on every machine and every run -
    // otherwise a script that depends on another having been loaded first
    // would work or not depending on the filesystem's whim.
    std::vector<std::filesystem::path> tPaths;

    for (const auto& tEntry : std::filesystem::directory_iterator(pDirectory, tErrorCode))
    {
        if (tEntry.is_regular_file() && tEntry.path().extension() == ".lua")
        {
            tPaths.push_back(tEntry.path());
        }
    }

    std::sort(tPaths.begin(), tPaths.end());

    // Dropped before re-running the files, so a reload replaces the handler
    // set instead of registering every one a second time.
    mHandlers.clear();

    bool tAllLoaded = true;

    for (const auto& tPath : tPaths)
    {
        // safe_script_file, not script_file: a syntax error in one content
        // file must not take the server down with it.
        auto tResult = mLua.safe_script_file(tPath.string(), sol::script_pass_on_error);

        if (!tResult.valid())
        {
            sol::error tError = tResult;
            std::osyncstream(std::cerr) << "[Script] Yuklenemedi " << tPath.filename().string()
                      << ": " << tError.what() << std::endl;
            tAllLoaded = false;
        }
    }

    std::osyncstream(std::cout) << "[Script] " << tPaths.size() << " dosya islendi: " << pDirectory << std::endl;

    return tAllLoaded;
}

template <typename... TArgs>
void ScriptEngine::callCharacterHook(const char* pName, Character& pCharacter, TArgs... pArgs)
{
    auto tIterator = mHandlers.find(pName);

    if (tIterator == mHandlers.end())
    {
        return; // nothing registered for this event - normal
    }

    mCharacterHandle->target = &pCharacter;

    for (sol::protected_function& tHandler : tIterator->second)
    {
        auto tResult = tHandler(mCharacterHandle, pArgs...);

        if (!tResult.valid())
        {
            sol::error tError = tResult;
            std::osyncstream(std::cerr) << "[Script] " << pName << " handler hatasi: " << tError.what() << std::endl;

            // Deliberately no break: the next handler still runs. One broken
            // quest must not silently disable every other piece of content
            // listening to the same event.
        }
    }

    // Cleared whether the calls succeeded or not, and before anything else
    // can run - this is what makes a stored handle safe.
    mCharacterHandle->target = nullptr;
}

void ScriptEngine::onEnterWorld(Character& pCharacter)
{
    callCharacterHook("enter_world", pCharacter);
}

void ScriptEngine::onMonsterKilled(Character& pCharacter, uint32_t pTemplateId)
{
    callCharacterHook("monster_killed", pCharacter, pTemplateId);
}

void ScriptEngine::onLevelUp(Character& pCharacter, uint32_t pNewLevel)
{
    callCharacterHook("level_up", pCharacter, pNewLevel);
}
