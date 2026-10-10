#include "CommandContext.h"

#include <cmath>
#include <charconv>

using namespace lakot;

CommandParseType CommandContext::parse(const std::string& pLine, CommandContext& pOut)
{
    if (pLine.empty() || pLine.front() != '/')
    {
        return CommandParseType::eNotCommand;
    }

    size_t tNameEnd = pLine.find(' ', 1);

    if (tNameEnd == std::string::npos)
    {
        tNameEnd = pLine.size();
    }

    std::string tName = pLine.substr(1, tNameEnd - 1);

    if (tName.empty() || tName.size() > kMaxNameLength)
    {
        return CommandParseType::eInvalidName;
    }

    for (char& tCharacter : tName)
    {
        if (tCharacter >= 'A' && tCharacter <= 'Z')
        {
            tCharacter = static_cast<char>(tCharacter - 'A' + 'a');
        }

        bool tIsValid = (tCharacter >= 'a' && tCharacter <= 'z') || (tCharacter >= '0' && tCharacter <= '9');

        if (!tIsValid)
        {
            return CommandParseType::eInvalidName;
        }
    }

    size_t tBegin = pLine.find_first_not_of(' ', tNameEnd);
    size_t tEnd = pLine.find_last_not_of(' ');

    pOut.name = std::move(tName);
    pOut.argumentText.clear();
    pOut.mSpans.clear();

    if (tBegin == std::string::npos)
    {
        return CommandParseType::eParsed;
    }

    pOut.argumentText = pLine.substr(tBegin, tEnd - tBegin + 1);

    size_t tPosition = 0;

    while (tPosition < pOut.argumentText.size() && pOut.mSpans.size() < kMaxArguments)
    {
        size_t tTokenEnd = pOut.argumentText.find(' ', tPosition);

        if (tTokenEnd == std::string::npos)
        {
            tTokenEnd = pOut.argumentText.size();
        }

        pOut.mSpans.emplace_back(tPosition, tTokenEnd - tPosition);

        size_t tNext = pOut.argumentText.find_first_not_of(' ', tTokenEnd);
        tPosition = tNext == std::string::npos ? pOut.argumentText.size() : tNext;
    }

    return CommandParseType::eParsed;
}

size_t CommandContext::getArgumentCount() const
{
    return mSpans.size();
}

std::string_view CommandContext::getArgument(size_t pIndex) const
{
    if (pIndex >= mSpans.size())
    {
        return {};
    }

    return std::string_view(argumentText).substr(mSpans[pIndex].first, mSpans[pIndex].second);
}

std::string CommandContext::getRest(size_t pIndex) const
{
    if (pIndex >= mSpans.size())
    {
        return {};
    }

    return argumentText.substr(mSpans[pIndex].first);
}

std::optional<int64_t> CommandContext::getInteger(size_t pIndex, int64_t pMin, int64_t pMax) const
{
    std::string_view tToken = getArgument(pIndex);
    int64_t tValue = 0;

    auto [tEnd, tError] = std::from_chars(tToken.data(), tToken.data() + tToken.size(), tValue);

    if (tToken.empty() || tError != std::errc() || tEnd != tToken.data() + tToken.size())
    {
        return std::nullopt;
    }

    if (tValue < pMin || tValue > pMax)
    {
        return std::nullopt;
    }

    return tValue;
}

std::optional<double> CommandContext::getNumber(size_t pIndex, double pMin, double pMax) const
{
    std::string_view tToken = getArgument(pIndex);
    double tValue = 0.0;

    auto [tEnd, tError] = std::from_chars(tToken.data(), tToken.data() + tToken.size(), tValue);

    if (tToken.empty() || tError != std::errc() || tEnd != tToken.data() + tToken.size())
    {
        return std::nullopt;
    }

    if (!std::isfinite(tValue) || tValue < pMin || tValue > pMax)
    {
        return std::nullopt;
    }

    return tValue;
}
