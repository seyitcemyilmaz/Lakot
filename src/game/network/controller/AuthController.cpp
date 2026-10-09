#include "AuthController.h"

#include "../NetworkManager.h"

using namespace lakot;

AuthController::AuthController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
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
    tRegister(protocol::Response::kResumeSessionResponse, &AuthController::handleResumeSessionResponse);
}

bool AuthController::sendOrReport(connection::Message& pMessage, const std::function<void()>& pOnNoConnection)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        if (pOnNoConnection)
        {
            pOnNoConnection();
        }

        return false;
    }

    // Every request carries an id, which the server echoes back as
    // ResponseHeader.reply_to - see NetworkManager::nextRequestId().
    pMessage.mutable_request()->mutable_header()->set_id(mNetworkManager->nextRequestId());

    tSession->send(pMessage);
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
            mRegisterResultCallback(false, "No connection to server!");
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
            mLoginResultCallback(false, "No connection to server!");
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

void AuthController::sendCharacterCreate(const std::string& pName, uint32_t pKingdom)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_character_create_request();
    tRequest->set_name(pName);
    tRequest->set_kingdom(pKingdom);

    sendOrReport(tMessage, [this]()
    {
        if (mCharacterCreateCallback)
        {
            mCharacterCreateCallback(false, "No connection to server!", {});
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
            mEnterWorldCallback(false, "No connection to server!", {});
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

void AuthController::clearCallbacks()
{
    mLoginResultCallback = nullptr;
    mRegisterResultCallback = nullptr;
    mCharacterListCallback = nullptr;
    mCharacterCreateCallback = nullptr;
    mEnterWorldCallback = nullptr;
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
        tMessage = tIsSuccess ? "Login successful." : "Invalid credentials!";
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
            mRegisterResultCallback(true, "Account created - you can now log in.");
            break;
        case services::auth::REGISTER_STATUS_EMAIL_IN_USE:
            mRegisterResultCallback(false, "That email is already registered.");
            break;
        case services::auth::REGISTER_STATUS_USERNAME_IN_USE:
            mRegisterResultCallback(false, "That username is taken.");
            break;
        default:
            mRegisterResultCallback(false, "Registration failed.");
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
        tCharacters.push_back({ tCharacter.character_id(), tCharacter.name(), tCharacter.map_id() });
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
        mCharacterCreateCallback(true, "", { tCharacter.character_id(), tCharacter.name(), tCharacter.map_id() });
        return;
    }

    std::string tMessage;

    switch (tResponse.status())
    {
        case services::auth::CHARACTER_CREATE_NAME_IN_USE:
            tMessage = "That name is already taken.";
            break;
        case services::auth::CHARACTER_CREATE_NAME_INVALID:
            tMessage = "Names must be 3-50 characters.";
            break;
        case services::auth::CHARACTER_CREATE_NO_FREE_SLOT:
            tMessage = "No free character slots.";
            break;
        case services::auth::CHARACTER_CREATE_INVALID_KINGDOM:
            tMessage = "Choose a kingdom for your first character.";
            break;
        default:
            tMessage = "Could not create character.";
            break;
    }

    mCharacterCreateCallback(false, tMessage, {});
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
        mEnterWorldCallback(false, "Could not enter the world.", {});
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

    const auto& tStats = tResponse.stats();
    tResult.stats.level = tStats.level();
    tResult.stats.experience = tStats.experience();
    tResult.stats.experienceForNextLevel = tStats.experience_for_next_level();
    tResult.stats.health = tStats.health();
    tResult.stats.maxHealth = tStats.max_health();
    tResult.stats.mana = tStats.mana();
    tResult.stats.maxMana = tStats.max_mana();
    tResult.stats.strength = tStats.strength();
    tResult.stats.dexterity = tStats.dexterity();
    tResult.stats.intelligence = tStats.intelligence();
    tResult.stats.vitality = tStats.vitality();
    tResult.stats.attack = tStats.attack();
    tResult.stats.defense = tStats.defense();
    tResult.stats.gold = tStats.gold();

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
