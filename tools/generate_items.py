"""Writes data/items.json: the warrior's tiered swords, armor and helmets plus the misc items."""

import json
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "items.json")

SWORDS = [
    "Iron Shortsword", "Hunnic Blade", "Gokturk Sabre", "Uyghur Sabre",
    "Karakhanid Sabre", "Seljuk Kilij", "Anatolian Pala", "Khagan's Kilij",
]
ARMORS = [
    "Steppe Herder Kaftan", "Hun Raider Vest", "Gokturk Guard Armor", "Uyghur Scale Armor",
    "Karakhanid Alp Armor", "Seljuk Emir Armor", "Anatolian Bey Kaftan", "Khagan's Guard Armor",
]
HELMETS = [
    "Felt Bork", "Raider Cap", "Gokturk Tolga", "Plumed Helm",
    "Karakhanid Helm", "Seljuk Helm", "Turban Helm", "Khagan's Wolf Helm",
]
TEMPERED_TINT = 0xD8E4FF

WEAPON, ARMOR, HELMET = 1, 2, 3
TYPE_EQUIPMENT, TYPE_CONSUMABLE, TYPE_MATERIAL, TYPE_QUEST = 1, 2, 3, 4


def equipment(template_id, name, slot, level, model, width, height, tint=0xFFFFFF, **bonus):
    return {
        "id": template_id, "name": name, "type": TYPE_EQUIPMENT, "equipSlot": slot, "maxStack": 1,
        "requiredLevel": level, "width": width, "height": height, "icon": "", "model": model, "tint": tint,
        "bonusAttack": bonus.get("attack", 0), "bonusDefense": bonus.get("defense", 0),
        "bonusMaxHealth": bonus.get("health", 0), "bonusMaxMana": bonus.get("mana", 0),
    }


def main():
    items = []

    for tier in range(16):
        family = tier // 2
        level = 1 if tier == 0 else tier * 5
        tempered = tier % 2 == 1
        name = ("Tempered " if tempered else "") + SWORDS[family]
        items.append(equipment(1001 + tier, name, WEAPON, level, f"models/weapons/sword_l{family * 10:02d}.glb".replace("l00", "l01"),
                               1, 3, TEMPERED_TINT if tempered else 0xFFFFFF, attack=4 + 6 * tier))

    for tier in range(8):
        level = 1 if tier == 0 else tier * 10
        suffix = f"l{tier * 10:02d}".replace("l00", "l01")
        items.append(equipment(2001 + tier, ARMORS[tier], ARMOR, level, f"models/warrior/armor_{suffix}.glb",
                               2, 3, defense=3 + 5 * tier, health=10 + 15 * tier))
        items.append(equipment(3001 + tier, HELMETS[tier], HELMET, level, f"models/helmets/helmet_{suffix}.glb",
                               2, 2, defense=2 + 3 * tier))

    items += [
        {"id": 4, "name": "Health Potion", "type": TYPE_CONSUMABLE, "equipSlot": 0, "maxStack": 20, "requiredLevel": 1,
         "width": 1, "height": 1, "icon": "potion.tga", "model": "", "tint": 0xFFFFFF},
        {"id": 5, "name": "Iron Ore", "type": TYPE_MATERIAL, "equipSlot": 0, "maxStack": 50, "requiredLevel": 1,
         "width": 1, "height": 1, "icon": "ore.tga", "model": "", "tint": 0xFFFFFF},
        {"id": 6, "name": "Ancient Relic", "type": TYPE_QUEST, "equipSlot": 0, "maxStack": 1, "requiredLevel": 1,
         "width": 1, "height": 2, "icon": "relic.tga", "model": "", "tint": 0xFFFFFF},
    ]

    items.sort(key=lambda item: item["id"])
    with open(OUT, "w", encoding="utf-8") as file:
        json.dump({"items": items}, file, indent=1)
    print("wrote", os.path.normpath(OUT), len(items), "items")


if __name__ == "__main__":
    main()
