#ifndef LAKOT_SERVER_COMMANDCONTEXT_H
#define LAKOT_SERVER_COMMANDCONTEXT_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lakot
{

enum class CommandParseType
{
    eParsed,
    eNotCommand,
    eInvalidName
};

struct CommandContext
{
    static constexpr size_t kMaxNameLength = 16;
    static constexpr size_t kMaxArguments = 16;

    uint64_t characterId = 0;
    uint64_t accountId = 0;
    std::string characterName;
    uint32_t gmLevel = 0;

    std::string name;
    std::string argumentText;

    static CommandParseType parse(const std::string& pLine, CommandContext& pOut);

    size_t getArgumentCount() const;
    std::string_view getArgument(size_t pIndex) const;
    std::string getRest(size_t pIndex) const;

    std::optional<int64_t> getInteger(size_t pIndex, int64_t pMin, int64_t pMax) const;
    std::optional<double> getNumber(size_t pIndex, double pMin, double pMax) const;

private:
    std::vector<std::pair<size_t, size_t>> mSpans;
};

}

#endif
