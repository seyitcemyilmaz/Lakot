#include "AuthController.h"

#include <syncstream>

#include "WorldController.h"
#include "../RequestLimits.h"

using namespace lakot;

void AuthController::sendRejection(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                   const connection::Message& pRequestMessage,
                                   common::StatusCode pStatusCode,
                                   const std::string& pMessage,
                                   bool pIsLogin)
{
    connection::Message tResponse;

    auto* tHeader = tResponse.mutable_response()->mutable_header();
    tHeader->set_reply_to(pRequestMessage.request().header().id());
    tHeader->mutable_status()->set_code(pStatusCode);
    tHeader->mutable_status()->set_message(pMessage);

    // Selecting the oneof case is load-bearing, not cosmetic: the client
    // dispatcher routes on payload_case(), so a response that sets none is
    // dropped as an unknown packet and the login screen hangs on "busy".
    if (pIsLogin)
    {
        tResponse.mutable_response()->mutable_login_response();
    }
    else
    {
        tResponse.mutable_response()->mutable_register_response();
    }

    pSession->send(tResponse);
}

AuthController::AuthController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager, WorldController& pWorldController)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
{

}

void AuthController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kRegisterRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleRegisterRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kLoginRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleLoginRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kResumeSessionRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleResumeSessionRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kLogoutRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleLogoutRequest(pSession, pMessage);
        }
    );
}

void AuthController::handleResumeSessionRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    connection::Message tResponse;
    tResponse.mutable_response()->mutable_header()->set_reply_to(pMessage.request().header().id());
    auto* tResume = tResponse.mutable_response()->mutable_resume_session_response();

    auto tResult = mNetworkManager.resumeSession(pSession, pMessage.request().resume_session_request().token());

    if (!tResult)
    {
        tResponse.mutable_response()->mutable_header()->mutable_status()->set_code(common::STATUS_UNAUTHORIZED);
        tResume->set_status(services::auth::RESUME_SESSION_FAILED);
        pSession->send(tResponse);
        return;
    }

    tResponse.mutable_response()->mutable_header()->mutable_status()->set_code(common::STATUS_OK);
    tResume->set_status(services::auth::RESUME_SESSION_OK);
    tResume->set_token(tResult->token);
    tResume->set_character_id(tResult->characterId);

    // Before the resync, so the client knows it is back before the zone's
    // full snapshot and inventory arrive.
    pSession->send(tResponse);

    if (tResult->characterId != 0)
    {
        mWorldController.resumeCharacter(tResult->characterId);
    }

    std::osyncstream(std::cout) << "[Server] Oturum surduruldu (hesap: " << tResult->accountId
              << ", karakter: " << tResult->characterId << ")" << std::endl;
}

void AuthController::handleLogoutRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    mNetworkManager.endSession(pSession);
}

void AuthController::handleRegisterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tRequest = pMessage.request().register_request();

    const std::string& tUsername = tRequest.username();
    const std::string& tPassword = tRequest.password();
    const std::string& tEmail = tRequest.email();

    // Bounded before anything expensive happens - no database round trip, and
    // in particular no password hashing, on input the server was never going
    // to accept. Previously only emptiness was checked, so an arbitrarily
    // long password reached PBKDF2 and an over-length username reached
    // Postgres to fail there as a generic database error.
    if (tUsername.size() < RequestLimits::kMinUsernameLength
        || tUsername.size() > RequestLimits::kMaxUsernameLength
        || !RequestLimits::hasOnlyPrintableCharacters(tUsername))
    {
        sendRejection(pSession, pMessage, common::STATUS_INVALID_REQUEST, "Invalid username.", false);
        return;
    }

    if (tPassword.size() < RequestLimits::kMinPasswordLength
        || tPassword.size() > RequestLimits::kMaxPasswordLength)
    {
        sendRejection(pSession, pMessage, common::STATUS_INVALID_REQUEST, "Invalid password.", false);
        return;
    }

    if (tEmail.empty()
        || tEmail.size() > RequestLimits::kMaxEmailLength
        || !RequestLimits::hasOnlyPrintableCharacters(tEmail))
    {
        sendRejection(pSession, pMessage, common::STATUS_INVALID_REQUEST, "Invalid email.", false);
        return;
    }

    mRepositoryManager.getAccountRepository().createAccount(tUsername, tPassword, tEmail,
    [pSession, pMessage, tUsername](AccountRepository::RegisterErrorType pErrorType, const std::string& pResult)
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());
        tHeader->mutable_status()->set_code(common::STATUS_OK);

        switch (pErrorType)
        {
            case AccountRepository::RegisterErrorType::eNoError:
            {
                tResponse.mutable_response()->mutable_register_response()->set_register_status(services::auth::REGISTER_STATUS_OK);
                break;
            }
            case AccountRepository::RegisterErrorType::eEmailInUse:
            {
                tResponse.mutable_response()->mutable_register_response()->set_register_status(services::auth::REGISTER_STATUS_EMAIL_IN_USE);
                break;
            }
            case AccountRepository::RegisterErrorType::eUsernameInUse:
            {
                tResponse.mutable_response()->mutable_register_response()->set_register_status(services::auth::REGISTER_STATUS_USERNAME_IN_USE);
                break;
            }
            case AccountRepository::RegisterErrorType::eDatabaseError:
            {
                tResponse.mutable_response()->mutable_register_response()->set_register_status(services::auth::REGISTER_STATUS_DATABASE_ERROR);
                break;
            }
            case AccountRepository::RegisterErrorType::eSystemError:
            {
                tResponse.mutable_response()->mutable_register_response()->set_register_status(services::auth::REGISTER_STATUS_SYSTEM_ERROR);
                break;
            }
            default:
            {
                tHeader->mutable_status()->set_code(common::STATUS_UNHANDLED_STATE);
            }
        }

        pSession->send(tResponse);
    });
}

void AuthController::handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tRequest = pMessage.request().login_request();
    std::string tUsername = tRequest.username();
    std::string tPassword = tRequest.password();

    if (pSession->getAccountId() != 0)
    {
        sendRejection(pSession, pMessage, common::STATUS_INVALID_REQUEST, "Already logged in.", true);
        return;
    }

    std::string tAddress = NetworkManager::getRemoteAddress(pSession);

    // Checked before the credentials are even looked at: a refused address
    // costs no database query and no password derivation, which is what makes
    // this a brute-force defence rather than just a message.
    if (!mAttemptLimiter.isAllowed(tAddress))
    {
        std::osyncstream(std::cout) << "[Server] Cok fazla basarisiz giris denemesi: " << tAddress << std::endl;

        sendRejection(pSession, pMessage, common::STATUS_UNAUTHORIZED,
                      "Too many failed attempts. Try again later.", true);
        return;
    }

    if (tUsername.empty() || tUsername.size() > RequestLimits::kMaxUsernameLength
        || tPassword.empty() || tPassword.size() > RequestLimits::kMaxPasswordLength)
    {
        // Counted as a failed attempt too - otherwise malformed requests are
        // a free way to probe without ever tripping the limit.
        mAttemptLimiter.recordFailure(tAddress);

        sendRejection(pSession, pMessage, common::STATUS_UNAUTHORIZED,
                      "Invalid username or password.", true);
        return;
    }

    mRepositoryManager.getAccountRepository().findByUsername(tUsername, tPassword,
    [this, pSession, pMessage, tUsername, tAddress](bool pIsSuccess, uint64_t pAccountId)
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());

        if (!pIsSuccess)
        {
            std::osyncstream(std::cout) << "[Server] Giris basarisiz: " << tUsername << std::endl;

            mAttemptLimiter.recordFailure(tAddress);

            tHeader->mutable_status()->set_code(common::STATUS_UNAUTHORIZED);
            tHeader->mutable_status()->set_message("Invalid username or password.");

            // Oneof case'i kLoginResponse yapmak icin bos bir login_response set ediliyor;
            // yoksa client'taki dispatcher payload_case()==0 gorup cevabi eslestiremiyor.
            tResponse.mutable_response()->mutable_login_response();

            pSession->send(tResponse);
            return;
        }

        std::osyncstream(std::cout) << "[Server] Giris Istegi: " << tUsername << " (Hesap: " << pAccountId << ")" << std::endl;

        // A success clears the address's history, so a player who mistyped a
        // few times and then got it right is not still one slip away from
        // being locked out.
        mAttemptLimiter.clear(tAddress);

        auto tToken = mNetworkManager.beginSession(pSession, pAccountId);

        if (!tToken)
        {
            if (pSession->isConnected())
            {
                sendRejection(pSession, pMessage, common::STATUS_INVALID_REQUEST, "Already logged in.", true);
            }

            return;
        }

        // Nothing enters the world here any more. Logging in authenticates an
        // account; which character exists in the world - and therefore where
        // anyone stands - is decided by the EnterWorld step that follows the
        // selection screen.
        tHeader->mutable_status()->set_code(common::STATUS_OK);
        tResponse.mutable_response()->mutable_login_response()->set_token(*tToken);

        pSession->send(tResponse);
    });
}
