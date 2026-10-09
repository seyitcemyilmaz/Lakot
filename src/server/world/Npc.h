#ifndef LAKOT_SERVER_NPC_H
#define LAKOT_SERVER_NPC_H

#include <cstdint>
#include <string>

namespace lakot
{

struct Npc
{
    uint64_t id = 0;
    std::string model;
    std::string role;
};

}

#endif
