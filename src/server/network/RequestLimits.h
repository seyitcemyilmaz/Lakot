#ifndef LAKOT_SERVER_REQUESTLIMITS_H
#define LAKOT_SERVER_REQUESTLIMITS_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace lakot
{

// Ceilings on anything that arrives off the wire as a string.
//
// NetworkSession only caps a whole frame at 10 MB, which is a transport-level
// guard against a malformed length prefix - not a statement about what any
// particular field may contain. Without per-field limits, a 10 MB chat
// message was accepted and then broadcast to every player in range (or, for a
// global message, to everyone on the server), turning one client's single
// request into an amplified flood. These are the values each request handler
// checks before doing any work.
namespace RequestLimits
{
    // Matches accounts.username VARCHAR(50) - a longer value would be
    // rejected by Postgres anyway, but as a database error rather than as
    // "your username is too long".
    constexpr size_t kMaxUsernameLength = 50;
    constexpr size_t kMinUsernameLength = 3;

    // Matches accounts.email VARCHAR(100).
    constexpr size_t kMaxEmailLength = 100;

    // The stored value is a PBKDF2 hash of fixed size, so this bounds work
    // done per login attempt rather than storage: hashing is deliberately
    // expensive, and an unbounded password would let one request burn
    // arbitrary CPU.
    constexpr size_t kMaxPasswordLength = 128;
    constexpr size_t kMinPasswordLength = 3;

    // Long enough for a sentence, short enough that broadcasting it to every
    // player in range stays cheap.
    constexpr size_t kMaxChatTextLength = 256;

    // Rejects control characters and stray newlines, which have no business
    // in a username or a chat line and would otherwise reach other players'
    // UI verbatim.
    inline bool hasOnlyPrintableCharacters(const std::string& pValue)
    {
        size_t tIndex = 0;

        while (tIndex < pValue.size())
        {
            unsigned char tLead = static_cast<unsigned char>(pValue[tIndex]);

            if (tLead < 0x80)
            {
                if (tLead < 0x20 || tLead == 0x7F)
                {
                    return false;
                }

                ++tIndex;
                continue;
            }

            size_t tLength = tLead >= 0xF0 && tLead <= 0xF4 ? 4 : tLead >= 0xE0 ? 3 : tLead >= 0xC2 && tLead <= 0xDF ? 2 : 0;

            if (tLength == 0 || tLead > 0xF4 || tIndex + tLength > pValue.size())
            {
                return false;
            }

            uint32_t tCodePoint = tLead & (0xFF >> (tLength + 1));

            for (size_t tOffset = 1; tOffset < tLength; ++tOffset)
            {
                unsigned char tNext = static_cast<unsigned char>(pValue[tIndex + tOffset]);

                if ((tNext & 0xC0) != 0x80)
                {
                    return false;
                }

                tCodePoint = (tCodePoint << 6) | (tNext & 0x3F);
            }

            bool tIsOverlong = (tLength == 3 && tCodePoint < 0x800) || (tLength == 4 && tCodePoint < 0x10000);
            bool tIsSurrogate = tCodePoint >= 0xD800 && tCodePoint <= 0xDFFF;
            bool tIsC1Control = tCodePoint >= 0x80 && tCodePoint <= 0x9F;

            if (tIsOverlong || tIsSurrogate || tIsC1Control || tCodePoint > 0x10FFFF)
            {
                return false;
            }

            tIndex += tLength;
        }

        return true;
    }

    inline bool isValidChatText(const std::string& pValue)
    {
        return !pValue.empty()
            && pValue.size() <= kMaxChatTextLength
            && hasOnlyPrintableCharacters(pValue);
    }
}

}

#endif
