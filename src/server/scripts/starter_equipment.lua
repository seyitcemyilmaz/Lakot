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
local SWORD_CLASSES = { [1] = true, [3] = true }

-- Registered, not declared as a global function of a well-known name: several
-- content files need to be able to react to the same event, and a global
-- would mean whichever file loaded last silently replaced all the others.
--
-- Fires every time a character enters the world - on login and after walking
-- through a portal. So it has to tell a brand new character from one that has
-- simply come back: the persistent starter-given flag is the test, so selling,
-- using or dropping the gear never earns a second kit.
lakot.on("enter_world", function(character)
    if character.is_starter_given then
        return
    end

    -- give_item returns how many did NOT fit. A brand new character has an
    -- empty bag so this cannot fail here, but checking it is the habit worth
    -- keeping: a quest reward handed to a full bag would otherwise vanish.
    local left_over = 0
    if SWORD_CLASSES[character.class_id] then
        left_over = left_over + character:give_item(WOODEN_SWORD, 1)
    end
    left_over = left_over +
        character:give_item(LEATHER_ARMOR, 1) +
        character:give_item(HEALTH_POTION, 5)

    if left_over > 0 then
        lakot.log(character.name .. ": bag was full, " .. left_over .. " item(s) not given")
        return
    end

    character:add_gold(100)
    character:mark_starter_given()

    lakot.log(character.name .. " received starter equipment (attack " .. character.attack .. ")")
end)
