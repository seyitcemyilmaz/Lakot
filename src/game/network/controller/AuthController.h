#ifndef LAKOT_AUTHCONTROLLER_H
#define LAKOT_AUTHCONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <connection.pb.h>

#include "../GameTypes.h"

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

// The client half of the pre-world flow: authenticate an account, list the
// characters on it, optionally create one, then enter the world as one of
// them. Logging in no longer says anything about where the player stands -
// that belongs to a character and arrives with EnterWorldResult.
class AuthController
{
public:
    // One row of the character selection screen.
    struct CharacterSummary
    {
        uint64_t characterId = 0;
        std::string name;
        uint32_t mapId = 0;
    };

    // Everything WorldScene needs to start: which character this session is
    // now playing, where it left off, and its full state.
    struct EnterWorldResult
    {
        uint64_t characterId = 0;
        std::string name;
        uint32_t mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yaw = 0.0f;

        CharacterStats stats;
        std::vector<OwnedItem> items;
    };

    using LoginResultCallback = std::function<void(bool pIsSuccess, const std::string& pMessage)>;
    using RegisterResultCallback = std::function<void(bool pIsSuccess, const std::string& pMessage)>;
    // pKingdom is the account's kingdom, 0 until its first character exists.
    using CharacterListCallback = std::function<void(const std::vector<CharacterSummary>& pCharacters, uint32_t pMaxSlots, uint32_t pKingdom)>;
    using CharacterCreateCallback = std::function<void(bool pIsSuccess, const std::string& pMessage, const CharacterSummary& pCharacter)>;
    using EnterWorldCallback = std::function<void(bool pIsSuccess, const std::string& pMessage, const EnterWorldResult& pResult)>;
    using ResumeResultCallback = std::function<void(bool pIsSuccess)>;

    AuthController(NetworkManager* pNetworkManager);

    void initialize();

    void sendRegisterAccount(const std::string& pUsername, const std::string& pPassword, const std::string& pEmail);
    void sendLoginRequest(const std::string& pUsername, const std::string& pPassword);

    void requestCharacterList();
    // pKingdom only matters for the account's first character.
    void sendCharacterCreate(const std::string& pName, uint32_t pKingdom);
    void sendEnterWorld(uint64_t pCharacterId);

    // Session resume token from the last login (rotated on every resume).
    // Empty when not logged in.
    bool hasSessionToken() const;
    void clearSessionToken();

    // Presents the stored token on the current connection.
    void sendResumeSession();

    // Tells the server this is an intentional logout, then forgets the token.
    void sendLogout();

    // Owned by NetworkManager for the whole process - not dropped by
    // clearCallbacks().
    void setResumeResultCallback(ResumeResultCallback pCallback);

    void setRegisterResultCallback(RegisterResultCallback pCallback);
    void setLoginResultCallback(LoginResultCallback pCallback);
    void setCharacterListCallback(CharacterListCallback pCallback);
    void setCharacterCreateCallback(CharacterCreateCallback pCallback);
    void setEnterWorldCallback(EnterWorldCallback pCallback);

    // Drops every callback above. They capture the scene/UI object that
    // registered them, but this controller lives as long as the process (it
    // is owned by Engine's NetworkManager) - so once that object is torn
    // down, a late or duplicate response would call into freed memory.
    // Called from LoginScene::exit() and CharacterSelectScene::exit(); only
    // one of them owns these callbacks at a time, and exit() always runs
    // before the next scene's enter().
    void clearCallbacks();

private:
    NetworkManager* mNetworkManager;

    LoginResultCallback mLoginResultCallback;
    RegisterResultCallback mRegisterResultCallback;
    CharacterListCallback mCharacterListCallback;
    CharacterCreateCallback mCharacterCreateCallback;
    EnterWorldCallback mEnterWorldCallback;
    ResumeResultCallback mResumeResultCallback;

    std::string mSessionToken;

    // Sends pMessage if there is a connection. Returns false (having already
    // reported the failure through pOnNoConnection) if there is not.
    bool sendOrReport(connection::Message& pMessage, const std::function<void()>& pOnNoConnection);

    void handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleRegisterResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleCharacterListResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleCharacterCreateResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleEnterWorldResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleResumeSessionResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
