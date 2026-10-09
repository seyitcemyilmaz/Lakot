#include "WhisperWindows.h"

#include <SDL3/SDL_log.h>

using namespace lakot;

WhisperWindows::WhisperWindows(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{

}

void WhisperWindows::open(uint64_t pPlayerId, const std::string& pUsername)
{
    ensureWindow(pPlayerId, pUsername).show();
}

void WhisperWindows::openBlank()
{
    if (mPendingBlankChatId != 0)
    {
        auto tIterator = mWindows.find(mPendingBlankChatId);

        if (tIterator != mWindows.end() && !tIterator->second->isTargetConfirmed())
        {
            tIterator->second->openForNewTarget();
            return;
        }

        mPendingBlankChatId = 0;
    }

    uint64_t tTempId = mNextTempChatId++;
    ensureWindow(tTempId, "").openForNewTarget();
    mPendingBlankChatId = tTempId;
}

void WhisperWindows::onMessageReceived(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText)
{
    adoptRealPlayerId(pFromPlayerId, pFromUsername);

    ensureWindow(pFromPlayerId, pFromUsername).appendIncoming(pFromUsername, pText);
}

void WhisperWindows::onSendResult(uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage)
{
    for (auto& [tPlayerId, tWindow] : mWindows)
    {
        tWindow->onSendResult(pRequestId, pIsSuccess, pMessage);
    }

    if (!pIsSuccess)
    {
        SDL_Log("Chat: %s", pMessage.c_str());
    }
}

void WhisperWindows::update()
{
    constexpr float kIconStackBaseTop = 12.0f;
    constexpr float kIconStackStep = 90.0f;

    int tIconSlot = 0;

    for (auto& [tPlayerId, tWindow] : mWindows)
    {
        tWindow->update();

        if (tWindow->isMinimized())
        {
            tWindow->setIconTopPosition(kIconStackBaseTop + kIconStackStep * static_cast<float>(tIconSlot));
            tIconSlot++;
        }
    }
}

bool WhisperWindows::minimizeOpen()
{
    bool tMinimizedAny = false;

    for (auto& [tPlayerId, tWindow] : mWindows)
    {
        if (tWindow->isFullyOpen())
        {
            tWindow->minimize();
            tMinimizedAny = true;
        }
    }

    return tMinimizedAny;
}

ChatRmlController& WhisperWindows::ensureWindow(uint64_t pPlayerId, const std::string& pUsername)
{
    auto tIterator = mWindows.find(pPlayerId);

    if (tIterator != mWindows.end())
    {
        return *tIterator->second;
    }

    constexpr float kBaseLeft = 80.0f;
    constexpr float kBaseTop = 80.0f;
    constexpr float kStaggerStep = 30.0f;
    constexpr int kStaggerWrap = 6;

    int tSlot = mStaggerCount % kStaggerWrap;
    float tLeft = kBaseLeft + kStaggerStep * static_cast<float>(tSlot);
    float tTop = kBaseTop + kStaggerStep * static_cast<float>(tSlot);
    mStaggerCount++;

    auto tWindow = std::make_unique<ChatRmlController>(mRmlUiLayer, pPlayerId, pUsername, tLeft, tTop);
    ChatRmlController& tRef = *tWindow;
    mWindows[pPlayerId] = std::move(tWindow);
    return tRef;
}

void WhisperWindows::adoptRealPlayerId(uint64_t pRealPlayerId, const std::string& pUsername)
{
    if (pUsername.empty() || mWindows.count(pRealPlayerId) != 0)
    {
        return;
    }

    for (auto tIterator = mWindows.begin(); tIterator != mWindows.end(); ++tIterator)
    {
        if (tIterator->first < kFirstTempChatId)
        {
            continue;
        }

        ChatRmlController& tWindow = *tIterator->second;

        if (!tWindow.isTargetConfirmed() || tWindow.getTargetUsername() != pUsername)
        {
            continue;
        }

        uint64_t tTempId = tIterator->first;

        tWindow.setTargetPlayerId(pRealPlayerId);

        auto tNode = mWindows.extract(tIterator);
        tNode.key() = pRealPlayerId;
        mWindows.insert(std::move(tNode));

        if (mPendingBlankChatId == tTempId)
        {
            mPendingBlankChatId = 0;
        }

        return;
    }
}
