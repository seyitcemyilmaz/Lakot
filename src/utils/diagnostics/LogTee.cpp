#include "diagnostics/LogTee.h"

#include "diagnostics/LogRing.h"

using namespace lakot;

LogTee::LogTee(std::streambuf& pTarget, LogRing& pRing)
    : mTarget(pTarget)
    , mRing(pRing)
{
}

LogTee::int_type LogTee::overflow(int_type pCharacter)
{
    if (traits_type::eq_int_type(pCharacter, traits_type::eof()))
    {
        return traits_type::not_eof(pCharacter);
    }

    const char tCharacter = traits_type::to_char_type(pCharacter);
    mRing.append(std::string_view(&tCharacter, 1));

    return mTarget.sputc(tCharacter);
}

std::streamsize LogTee::xsputn(const char* pText, std::streamsize pCount)
{
    mRing.append(std::string_view(pText, static_cast<std::size_t>(pCount)));

    return mTarget.sputn(pText, pCount);
}

int LogTee::sync()
{
    return mTarget.pubsync();
}
