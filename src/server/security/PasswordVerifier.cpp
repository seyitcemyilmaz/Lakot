#include "PasswordVerifier.h"

#include <iostream>
#include <syncstream>

#include "PasswordHasher.h"

using namespace lakot;

PasswordVerifier::~PasswordVerifier()
{
    mPool.join();
}

PasswordVerifier::PasswordVerifier()
    : mPool(kThreadCount)
{
}

void PasswordVerifier::verifyAsync(std::string pPassword, std::string pStoredHash, VerifyCallback pCallback)
{
    boost::asio::post(mPool, [tPassword = std::move(pPassword), tStoredHash = std::move(pStoredHash), tCallback = std::move(pCallback)]()
    {
        bool tIsMatch = false;

        try
        {
            tIsMatch = PasswordHasher::verify(tPassword, tStoredHash);
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[PasswordVerifier] Verification error: " << tException.what() << std::endl;
        }

        if (tCallback)
        {
            tCallback(tIsMatch);
        }
    });
}
