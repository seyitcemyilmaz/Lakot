#ifndef LAKOT_SERVER_PASSWORDHASHER_H
#define LAKOT_SERVER_PASSWORDHASHER_H

#include <string>

namespace lakot
{

// Salted, iterated password hashing (PBKDF2-HMAC-SHA256, via OpenSSL - the
// project already links libcrypto transitively through libpqxx, so this
// reuses what's already in the environment rather than adding a new
// dependency). Never store or compare raw passwords.
namespace PasswordHasher
{
    // Returns a self-describing encoded string
    // ("pbkdf2_sha256$<iterations>$<hex salt>$<hex hash>") safe to store
    // directly in the accounts.password column.
    std::string hash(const std::string& pPassword);

    // Recomputes the hash using the salt/iteration count embedded in
    // pStoredHash and compares in constant time. False for any malformed
    // or non-matching stored value.
    bool verify(const std::string& pPassword, const std::string& pStoredHash);
}

}

#endif
