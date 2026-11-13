#ifndef LAKOT_SERVER_NPC_H
#define LAKOT_SERVER_NPC_H

#include <cstdint>
#include <string>
#include <vector>

namespace lakot
{

struct Npc
{
    uint64_t id = 0;
    std::string model;
    std::string role;
    std::string name;
    std::vector<uint32_t> shopItems;
};

}

#endif
