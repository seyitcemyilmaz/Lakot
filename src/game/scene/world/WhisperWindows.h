#ifndef LAKOT_WHISPER_WINDOWS_H
#define LAKOT_WHISPER_WINDOWS_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../gui/rml/ChatRmlController.h"

namespace lakot
{

class RmlUiLayer;

class WhisperWindows
{
public:
    explicit WhisperWindows(RmlUiLayer& pRmlUiLayer);

    void open(uint64_t pPlayerId, const std::string& pUsername);

    void openBlank();

    void onMessageReceived(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText);
    void onSendResult(uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage);

    void update();

    bool minimizeOpen();

private:
    static constexpr uint64_t kFirstTempChatId = 0x8000000000000000ULL;

    RmlUiLayer& mRmlUiLayer;

    std::unordered_map<uint64_t, std::unique_ptr<ChatRmlController>> mWindows;
    int mStaggerCount{0};

    uint64_t mNextTempChatId{kFirstTempChatId};
    uint64_t mPendingBlankChatId{0};

    ChatRmlController& ensureWindow(uint64_t pPlayerId, const std::string& pUsername);
    void adoptRealPlayerId(uint64_t pRealPlayerId, const std::string& pUsername);
};

}

#endif
