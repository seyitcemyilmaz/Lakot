#include "AuthController.h"

#include "../NetworkManager.h"
#include "../StatsMessage.h"
#include "../../localization/Localization.h"

using namespace lakot;

AuthController::AuthController(NetworkManager* pNetworkManager, const Localization& pLocalization)
    : mNetworkManager(pNetworkManager)
    , mLocalization(pLocalization)
{

}

void AuthController::initialize()
{
    auto tRegister = [this](int pCaseId, void (AuthController::*pHandler)(std::shared_ptr<NetworkSession<connection::Message>>, const connection::Message&))
    {
        mNetworkManager->getDispatcher().registerHandler(pCaseId, PacketType::Response,
            [this, pHandler](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
            {
                (this->*pHandler)(pSession, pMessage);
            }
        );
    };

    tRegister(protocol::Response::kLoginResponse, &AuthController::handleLoginResponse);
    tRegister(protocol::Response::kRegisterResponse, &AuthController::handleRegisterResponse);
    tRegister(protocol::Response::kCharacterListResponse, &AuthController::handleCharacterListResponse);
    tRegister(protocol::Response::kCharacterCreateResponse, &AuthController::handleCharacterCreateResponse);
    tRegister(protocol::Response::kEnterWorldResponse, &AuthController::handleEnterWorldResponse);
    tRegister(protocol::Response::kDeleteCharacterResponse, &AuthController::handleCharacterDeleteResponse);
    tRegister(protocol::Response::kResumeSessionResponse, &AuthController::handleResumeSessionResponse);
}

bool AuthController::sendOrReport(connection::Message& pMessage, const std::function<void()>& pOnNoConnection)
{
    if (mNetworkManager->sendRequest(pMessage) == 0)
    {
        if (pOnNoConnection)
        {
            pOnNoConnection();
        }

        return false;
    }

    return true;
}

void AuthController::sendRegisterAccount(const std::string& pUsername, const std::string& pPassword, const std::string& pEmail)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_register_request();
    tRequest->set_username(pUsername);
    tRequest->set_password(pPassword);
    tRequest->set_email(pEmail);

    sendOrReport(tMessage, [this]()
    {
        if (mRegisterResultCallback)
        {
            mRegisterResultCallback(false, mLocalization.text("auth.error.no_connection"));
        }
    });
}

void AuthController::sendLoginRequest(const std::string& pUsername, const std::string& pPassword)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_login_request();
    tRequest->set_username(pUsername);
    tRequest->set_password(pPassword);

    sendOrReport(tMessage, [this]()
    {
        if (mLoginResultCallback)
        {
            mLoginResultCallback(false, mLocalization.text("auth.error.no_connection"));
        }
    });
}

void AuthController::requestCharacterList()
{
    connection::Message tMessage;
    tMessage.mutable_request()->mutable_character_list_request();

    sendOrReport(tMessage, [this]()
    {
        if (mCharacterListCallback)
        {
            // An empty list with zero slots is how the selection screen shows
            // "nothing to choose from" - there is no separate error path for
            // a listing that never arrived.
            mCharacterListCallback({}, 0, 0);
        }
    });
}

void AuthController::sendCharacterCreate(const std::string& pName, uint32_t pKingdom, uint32_t pClassId)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_character_create_request();
    tRequest->set_name(pName);
    tRequest->set_kingdom(pKingdom);
    tRequest->set_class_id(pClassId);

    sendOrReport(tMessage, [this]()
    {
        if (mCharacterCreateCallback)
        {
            mCharacterCreateCallback(false, mLocalization.text("auth.error.no_connection"), {});
        }
    });
}

void AuthController::sendEnterWorld(uint64_t pCharacterId)
{
    connection::Message tMessage;
    tMessage.mutable_request()->mutable_enter_world_request()->set_character_id(pCharacterId);

    sendOrReport(tMessage, [this]()
    {
        if (mEnterWorldCallback)
        {
            mEnterWorldCallback(false, mLocalization.text("auth.error.no_connection"), {});
        }
    });
}

void AuthController::sendDeleteCharacter(uint64_t pCharacterId, const std::string& pConfirmName)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_delete_character_request();
    tRequest->set_character_id(pCharacterId);
    tRequest->set_confirm_name(pConfirmName);

    sendOrReport(tMessage, [this, pCharacterId]()
    {
        if (mCharacterDeleteCallback)
        {
            mCharacterDeleteCallback(false, mLocalization.text("auth.error.no_connection"), pCharacterId);
        }
    });
}

bool AuthController::hasSessionToken() const
{
    return !mSessionToken.empty();
}

void AuthController::clearSessionToken()
{
    mSessionToken.clear();
}

void AuthController::sendResumeSession()
{
    connection::Message tMessage;
    tMessage.mutable_request()->mutable_resume_session_request()->set_token(mSessionToken);

    sendOrReport(tMessage, nullptr);
}

void AuthController::sendLogout()
{
    connection::Message tMessage;
    tMessage.mutable_request()->mutable_logout_request();

    sendOrReport(tMessage, nullptr);

    mSessionToken.clear();
}

void AuthController::setResumeResultCallback(ResumeResultCallback pCallback)
{
    mResumeResultCallback = std::move(pCallback);
}

void AuthController::setRegisterResultCallback(RegisterResultCallback pCallback)
{
    mRegisterResultCallback = pCallback;
}

void AuthController::setLoginResultCallback(LoginResultCallback pCallback)
{
    mLoginResultCallback = pCallback;
}

void AuthController::setCharacterListCallback(CharacterListCallback pCallback)
{
    mCharacterListCallback = pCallback;
}

void AuthController::setCharacterCreateCallback(CharacterCreateCallback pCallback)
{
    mCharacterCreateCallback = pCallback;
}

void AuthController::setEnterWorldCallback(EnterWorldCallback pCallback)
{
    mEnterWorldCallback = pCallback;
}

void AuthController::setCharacterDeleteCallback(CharacterDeleteCallback pCallback)
{
    mCharacterDeleteCallback = pCallback;
}

void AuthController::clearCallbacks()
{
    mLoginResultCallback = nullptr;
    mRegisterResultCallback = nullptr;
    mCharacterListCallback = nullptr;
    mCharacterCreateCallback = nullptr;
    mEnterWorldCallback = nullptr;
    mCharacterDeleteCallback = nullptr;
}

void AuthController::handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    bool tIsSuccess = pMessage.response().header().status().code() == common::STATUS_OK;

    // Kept even with no callback registered - the token is what lets a
    // dropped connection resume later.
    if (tIsSuccess)
    {
        mSessionToken = pMessage.response().login_response().token();
    }

    if (!mLoginResultCallback)
    {
        return;
    }

    std::string tMessage = pMessage.response().header().status().message();

    if (tMessage.empty())
    {
        tMessage = tIsSuccess ? mLocalization.text("auth.login.success") : mLocalization.text("auth.login.invalid_credentials");
    }

    mLoginResultCallback(tIsSuccess, tMessage);
}

void AuthController::handleRegisterResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mRegisterResultCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().register_response();

    switch (tResponse.register_status())
    {
        case services::auth::REGISTER_STATUS_OK:
            mRegisterResultCallback(true, mLocalization.text("auth.register.created"));
            break;
        case services::auth::REGISTER_STATUS_EMAIL_IN_USE:
            mRegisterResultCallback(false, mLocalization.text("auth.register.email_in_use"));
            break;
        case services::auth::REGISTER_STATUS_USERNAME_IN_USE:
            mRegisterResultCallback(false, mLocalization.text("auth.register.username_taken"));
            break;
        default:
            mRegisterResultCallback(false, mLocalization.text("auth.register.failed"));
            break;
    }
}

void AuthController::handleCharacterListResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mCharacterListCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().character_list_response();

    std::vector<CharacterSummary> tCharacters;
    tCharacters.reserve(tResponse.characters_size());

    for (const auto& tCharacter : tResponse.characters())
    {
        tCharacters.push_back({ tCharacter.character_id(), tCharacter.name(), tCharacter.map_id(), tCharacter.class_id(), tCharacter.level() });
    }

    mCharacterListCallback(tCharacters, tResponse.max_slots(), tResponse.kingdom());
}

void AuthController::handleCharacterCreateResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mCharacterCreateCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().character_create_response();

    if (tResponse.status() == services::auth::CHARACTER_CREATE_OK)
    {
        const auto& tCharacter = tResponse.character();
        mCharacterCreateCallback(true, "", { tCharacter.character_id(), tCharacter.name(), tCharacter.map_id(), tCharacter.class_id(), tCharacter.level() });
        return;
    }

    std::string tMessage;

    switch (tResponse.status())
    {
        case services::auth::CHARACTER_CREATE_NAME_IN_USE:
            tMessage = mLocalization.text("auth.character.name_in_use");
            break;
        case services::auth::CHARACTER_CREATE_NAME_INVALID:
            tMessage = mLocalization.text("auth.character.name_invalid");
            break;
        case services::auth::CHARACTER_CREATE_NO_FREE_SLOT:
            tMessage = mLocalization.text("auth.character.no_free_slot");
            break;
        case services::auth::CHARACTER_CREATE_INVALID_KINGDOM:
            tMessage = mLocalization.text("auth.character.invalid_kingdom");
            break;
        case services::auth::CHARACTER_CREATE_INVALID_CLASS:
            tMessage = mLocalization.text("auth.character.invalid_class");
            break;
        default:
            tMessage = mLocalization.text("auth.character.create_failed");
            break;
    }

    mCharacterCreateCallback(false, tMessage, {});
}

void AuthController::handleCharacterDeleteResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mCharacterDeleteCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().delete_character_response();
    uint64_t tCharacterId = tResponse.character_id();

    std::string tMessage;

    switch (tResponse.status())
    {
        case services::auth::DELETE_CHARACTER_OK:
            mCharacterDeleteCallback(true, "", tCharacterId);
            return;
        case services::auth::DELETE_CHARACTER_NOT_FOUND:
            tMessage = mLocalization.text("auth.delete.not_found");
            break;
        case services::auth::DELETE_CHARACTER_NAME_MISMATCH:
            tMessage = mLocalization.text("auth.delete.name_mismatch");
            break;
        case services::auth::DELETE_CHARACTER_IN_WORLD:
            tMessage = mLocalization.text("auth.delete.in_world");
            break;
        default:
            tMessage = mLocalization.text("auth.delete.failed");
            break;
    }

    mCharacterDeleteCallback(false, tMessage, tCharacterId);
}

void AuthController::handleEnterWorldResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mEnterWorldCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().enter_world_response();

    if (tResponse.status() != services::auth::ENTER_WORLD_OK)
    {
        mEnterWorldCallback(false, mLocalization.text("auth.enter_world.failed"), {});
        return;
    }

    EnterWorldResult tResult;
    tResult.characterId = tResponse.character_id();
    tResult.name = tResponse.name();
    tResult.mapId = tResponse.map_id();
    tResult.x = tResponse.pos_x();
    tResult.y = tResponse.pos_y();
    tResult.z = tResponse.pos_z();
    tResult.yaw = tResponse.yaw();

    tResult.stats = StatsMessage::read(tResponse.stats());

    tResult.items.reserve(tResponse.items_size());

    for (const auto& tItem : tResponse.items())
    {
        tResult.items.push_back({ tItem.template_id(), tItem.count(),
                                  static_cast<ItemLocationType>(tItem.location()), tItem.slot() });
    }

    mEnterWorldCallback(true, "", tResult);
}

void AuthController::handleResumeSessionResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tResponse = pMessage.response().resume_session_response();

    bool tIsSuccess = tResponse.status() == services::auth::RESUME_SESSION_OK;

    if (tIsSuccess)
    {
        mSessionToken = tResponse.token();
    }
    else
    {
        mSessionToken.clear();
    }

    if (mResumeResultCallback)
    {
        mResumeResultCallback(tIsSuccess);
    }
}
