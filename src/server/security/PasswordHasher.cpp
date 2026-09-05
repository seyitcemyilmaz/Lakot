#include "PasswordHasher.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace lakot;

namespace
{
    constexpr int kSaltBytes = 16;
    constexpr int kHashBytes = 32;
    constexpr int kIterations = 100000;
    const char* kAlgorithmTag = "pbkdf2_sha256";

    std::string toHex(const unsigned char* pData, size_t pLength)
    {
        static const char tHexChars[] = "0123456789abcdef";
        std::string tResult;
        tResult.reserve(pLength * 2);

        for (size_t i = 0; i < pLength; ++i)
        {
            tResult.push_back(tHexChars[(pData[i] >> 4) & 0xF]);
            tResult.push_back(tHexChars[pData[i] & 0xF]);
        }

        return tResult;
    }

    int hexValue(char pChar)
    {
        if (pChar >= '0' && pChar <= '9') return pChar - '0';
        if (pChar >= 'a' && pChar <= 'f') return pChar - 'a' + 10;
        if (pChar >= 'A' && pChar <= 'F') return pChar - 'A' + 10;
        return -1;
    }

    std::vector<unsigned char> fromHex(const std::string& pHex)
    {
        if (pHex.size() % 2 != 0)
        {
            return {};
        }

        std::vector<unsigned char> tResult;
        tResult.reserve(pHex.size() / 2);

        for (size_t i = 0; i + 1 < pHex.size(); i += 2)
        {
            int tHigh = hexValue(pHex[i]);
            int tLow = hexValue(pHex[i + 1]);

            if (tHigh < 0 || tLow < 0)
            {
                return {};
            }

            tResult.push_back(static_cast<unsigned char>((tHigh << 4) | tLow));
        }

        return tResult;
    }

    std::vector<unsigned char> derive(const std::string& pPassword, const std::vector<unsigned char>& pSalt, int pIterations)
    {
        std::vector<unsigned char> tHash(kHashBytes);

        PKCS5_PBKDF2_HMAC(
            pPassword.data(), static_cast<int>(pPassword.size()),
            pSalt.data(), static_cast<int>(pSalt.size()),
            pIterations,
            EVP_sha256(),
            kHashBytes, tHash.data());

        return tHash;
    }

    bool constantTimeEquals(const std::vector<unsigned char>& pA, const std::vector<unsigned char>& pB)
    {
        if (pA.size() != pB.size())
        {
            return false;
        }

        unsigned char tDiff = 0;

        for (size_t i = 0; i < pA.size(); ++i)
        {
            tDiff |= pA[i] ^ pB[i];
        }

        return tDiff == 0;
    }
}

std::string PasswordHasher::hash(const std::string& pPassword)
{
    std::vector<unsigned char> tSalt(kSaltBytes);
    RAND_bytes(tSalt.data(), kSaltBytes);

    std::vector<unsigned char> tHash = derive(pPassword, tSalt, kIterations);

    std::ostringstream tStream;
    tStream << kAlgorithmTag << '$' << kIterations << '$'
            << toHex(tSalt.data(), tSalt.size()) << '$'
            << toHex(tHash.data(), tHash.size());

    return tStream.str();
}

bool PasswordHasher::verify(const std::string& pPassword, const std::string& pStoredHash)
{
    size_t tFirstDollar = pStoredHash.find('$');
    size_t tSecondDollar = (tFirstDollar == std::string::npos) ? std::string::npos : pStoredHash.find('$', tFirstDollar + 1);
    size_t tThirdDollar = (tSecondDollar == std::string::npos) ? std::string::npos : pStoredHash.find('$', tSecondDollar + 1);

    if (tFirstDollar == std::string::npos || tSecondDollar == std::string::npos || tThirdDollar == std::string::npos)
    {
        return false; // not a recognized hash format (e.g. legacy plaintext row)
    }

    std::string tAlgorithm = pStoredHash.substr(0, tFirstDollar);

    if (tAlgorithm != kAlgorithmTag)
    {
        return false;
    }

    int tIterations = std::atoi(pStoredHash.substr(tFirstDollar + 1, tSecondDollar - tFirstDollar - 1).c_str());
    std::string tSaltHex = pStoredHash.substr(tSecondDollar + 1, tThirdDollar - tSecondDollar - 1);
    std::string tHashHex = pStoredHash.substr(tThirdDollar + 1);

    std::vector<unsigned char> tSalt = fromHex(tSaltHex);
    std::vector<unsigned char> tExpectedHash = fromHex(tHashHex);

    if (tSalt.empty() || tExpectedHash.empty() || tIterations <= 0)
    {
        return false;
    }

    std::vector<unsigned char> tActualHash = derive(pPassword, tSalt, tIterations);

    return constantTimeEquals(tActualHash, tExpectedHash);
}
