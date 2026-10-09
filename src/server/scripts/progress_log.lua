lakot.on("monster_killed", function(character, template_id)
    lakot.log("monster_killed " .. character.name .. " template=" .. template_id)
end)

lakot.on("level_up", function(character, new_level)
    lakot.log("level_up " .. character.name .. " level=" .. new_level)
end)
