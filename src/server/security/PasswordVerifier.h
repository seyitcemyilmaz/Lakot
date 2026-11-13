#ifndef LAKOT_SERVER_PASSWORDVERIFIER_H
#define LAKOT_SERVER_PASSWORDVERIFIER_H

#include <functional>
#include <string>

#include <boost/asio.hpp>

namespace lakot
{

class PasswordVerifier
{
public:
    using VerifyCallback = std::function<void(bool pIsMatch)>;

    virtual ~PasswordVerifier();
    PasswordVerifier();

    void verifyAsync(std::string pPassword, std::string pStoredHash, VerifyCallback pCallback);

private:
    static constexpr size_t kThreadCount = 2;

    boost::asio::thread_pool mPool;
};

}

#endif
