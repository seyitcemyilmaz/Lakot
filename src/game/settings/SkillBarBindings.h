#ifndef LAKOT_SKILL_BAR_BINDINGS_H
#define LAKOT_SKILL_BAR_BINDINGS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace lakot
{

class SkillBarBindings
{
public:
    static constexpr size_t kSlotCount = 10;

    explicit SkillBarBindings(uint64_t pCharacterId);

    std::optional<uint32_t> getTemplateId(size_t pSlot) const;

    void setTemplateId(size_t pSlot, uint32_t pTemplateId);
    void clear(size_t pSlot);

private:
    uint64_t mCharacterId;
    std::array<uint32_t, kSlotCount> mTemplateIds{};

    void load();
    void save() const;

    std::string getFilePath() const;
};

}

#endif
