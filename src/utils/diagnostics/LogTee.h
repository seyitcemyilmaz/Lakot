#ifndef LAKOT_LOGTEE_H
#define LAKOT_LOGTEE_H

#include <streambuf>

namespace lakot {

class LogRing;

class LogTee : public std::streambuf
{
public:
    LogTee(std::streambuf& pTarget, LogRing& pRing);

protected:
    int_type overflow(int_type pCharacter) override;
    std::streamsize xsputn(const char* pText, std::streamsize pCount) override;
    int sync() override;

private:
    std::streambuf& mTarget;
    LogRing& mRing;
};

}

#endif
