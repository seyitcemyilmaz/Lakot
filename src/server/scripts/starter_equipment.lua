-- Starter equipment.
--
-- The first real piece of game content written outside C++, and the first
-- caller of Character::giveItem. Changing what a new character starts with,
-- or removing this entirely, is an edit to this file and a server restart -
-- no recompile, and eventually not even a restart.
--
-- Item ids come from data/items.json (tools/generate_items.py):
--   1001 Iron Shortsword   2001 Steppe Herder Kaftan   3001 Felt Bork
--   4 Health Potion  5 Iron Ore        6 Ancient Relic

local WOODEN_SWORD  = 1001
local LEATHER_ARMOR = 2001
local HEALTH_POTION = 4

-- Registered, not declared as a global function of a well-known name: several
-- content files need to be able to react to the same event, and a global
-- would mean whichever file loaded last silently replaced all the others.
--
-- Fires every time a character enters the world - on login and after walking
-- through a portal. So it has to tell a brand new character from one that has
-- simply come back, and "owns nothing at all" is the test: anyone who has
-- ever been given this gear fails it, including someone who sold part of it.
lakot.on("enter_world", function(character)
    local owns_anything =
        character:count_item(WOODEN_SWORD) > 0 or
        character:count_item(LEATHER_ARMOR) > 0 or
        character:count_item(HEALTH_POTION) > 0

    if owns_anything then
        return
    end

    -- give_item returns how many did NOT fit. A brand new character has an
    -- empty bag so this cannot fail here, but checking it is the habit worth
    -- keeping: a quest reward handed to a full bag would otherwise vanish.
    local left_over =
        character:give_item(WOODEN_SWORD, 1) +
        character:give_item(LEATHER_ARMOR, 1) +
        character:give_item(HEALTH_POTION, 5)

    if left_over > 0 then
        lakot.log(character.name .. ": bag was full, " .. left_over .. " item(s) not given")
        return
    end

    character:add_gold(100)

    lakot.log(character.name .. " received starter equipment (attack " .. character.attack .. ")")
end)
