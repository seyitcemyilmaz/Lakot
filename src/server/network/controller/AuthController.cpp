#include "AuthController.h"

#include "WorldController.h"

using namespace lakot;

AuthController::AuthController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager)
    : BaseController(pNetworkManager, pRepositoryManager)
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
}

void AuthController::setWorldController(WorldController& pWorldController)
{
    mWorldController = &pWorldController;
}

void AuthController::handleRegisterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tRequest = pMessage.request().register_request();

    const std::string& tUsername = tRequest.username();

    if (tUsername.empty())
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());
        tHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tHeader->mutable_status()->set_message("Username cannot be empty.");

        tResponse.mutable_response()->mutable_register_response();

        pSession->send(tResponse);
        return;
    }

    const std::string& tPassword = tRequest.password();

    if (tPassword.empty())
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());
        tHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tHeader->mutable_status()->set_message("Password cannot be empty.");

        tResponse.mutable_response()->mutable_register_response();

        pSession->send(tResponse);
        return;
    }

    const std::string& tEmail = tRequest.email();

    if (tEmail.empty())
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());
        tHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tHeader->mutable_status()->set_message("Email cannot be empty.");

        tResponse.mutable_response()->mutable_register_response();

        pSession->send(tResponse);
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

    mRepositoryManager.getAccountRepository().findByUsername(tUsername, tPassword,
    [this, pSession, pMessage, tUsername](bool pIsSuccess, uint64_t pUserId, const AccountRepository::PlayerSpawnState& pSpawnState)
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pMessage.request().header().id());

        if (!pIsSuccess)
        {
            std::cout << "[Server] Giris basarisiz: " << tUsername << std::endl;

            tHeader->mutable_status()->set_code(common::STATUS_UNAUTHORIZED);
            tHeader->mutable_status()->set_message("Invalid username or password.");

            // Oneof case'i kLoginResponse yapmak icin bos bir login_response set ediliyor;
            // yoksa client'taki dispatcher payload_case()==0 gorup cevabi eslestiremiyor.
            tResponse.mutable_response()->mutable_login_response();

            pSession->send(tResponse);
            return;
        }

        std::cout << "[Server] Giris Istegi: " << tUsername << " (ID: " << pUserId << ")" << std::endl;

        pSession->setUserId(pUserId);

        auto tOldSession = mNetworkManager.getSessionRegistry().addOrReplace(pUserId, pSession);

        if (tOldSession)
        {
            if (tOldSession != pSession)
            {
                std::cout << "[Server] Cakisman oturum. Eski baglanti kapatiliyor." << std::endl;
                tOldSession->close();
            }
        }

        // So the player's very first PlayerStateUpdate (sent from the
        // restored position below) lands in the correct map's registry
        // instead of WorldController defaulting them to Town.
        if (mWorldController)
        {
            mWorldController->seedPlayerMap(pUserId, static_cast<services::world::MapId>(pSpawnState.mapId));
            mWorldController->setPlayerUsername(pUserId, tUsername);
        }

        tHeader->mutable_status()->set_code(common::STATUS_OK);

        auto* tLoginResponse = tResponse.mutable_response()->mutable_login_response();
        tLoginResponse->set_token("TOKEN_LAKOT_123");
        tLoginResponse->set_map_id(pSpawnState.mapId);
        tLoginResponse->set_pos_x(pSpawnState.x);
        tLoginResponse->set_pos_y(pSpawnState.y);
        tLoginResponse->set_pos_z(pSpawnState.z);
        tLoginResponse->set_yaw(pSpawnState.yaw);

        pSession->send(tResponse);
    });
}
