#ifndef LAKOT_SERVER_ITEMCATALOG_H
#define LAKOT_SERVER_ITEMCATALOG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ItemTemplate.h"

namespace lakot
{

// Every item definition in the game, read-only.
//
// Filled exactly once, during server startup, before the network layer starts
// accepting connections - and never written to again. That immutability is
// the whole design: it means every zone thread, and later every Lua state,
// can read templates concurrently with no lock and no copy, the same way
// WorldController::mZones is safe to read because it is fixed at construction.
//
// Anything that would need to mutate the catalog at runtime (a live content
// reload) must not add a setter here; it would have to swap a whole new
// catalog in behind a pointer, so readers never observe a half-updated table.
class ItemCatalog
{
public:
    // Returns nullptr for an unknown id. Callers are expected to treat that
    // as "this item row is corrupt or references a deleted template" and skip
    // it, rather than as an assertion failure - item_instances rows outlive
    // any particular version of the template table.
    const ItemTemplate* find(uint32_t pTemplateId) const;

    bool isEmpty() const;
    size_t size() const;

    // Every template, for sending the catalog to a client. Ordered by id so
    // the list a client receives is stable across sessions and servers.
    const std::vector<ItemTemplate>& getAll() const;

    // Called only by the startup loader (ItemRepository). Not thread safe and
    // not meant to be - it must have finished before anything else runs.
    void setTemplates(std::vector<ItemTemplate> pTemplates);

    bool load(const std::string& pDataDirectory, std::string& pError);

private:
    std::unordered_map<uint32_t, ItemTemplate> mTemplates;

    // The same templates in id order. Kept alongside the map rather than
    // rebuilt on demand because sending the catalog happens once per player
    // login, and an unordered_map has no stable order to send.
    std::vector<ItemTemplate> mOrdered;
};

}

#endif
