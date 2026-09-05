#ifndef LAKOT_CHATPANEL_H
#define LAKOT_CHATPANEL_H

#include <cstdint>
#include <vector>

#include "../Panel.h"

namespace lakot
{

// A single private conversation window with one other player, opened by
// WorldScene (via right-click -> context menu -> Chat, or automatically on
// an incoming whisper). One instance per remote player id - the window's
// name embeds the target's username, so several can be open side by side.
struct ChatPanel : public Panel
{
    struct ChatLine
    {
        bool isLocal;
        std::string author;
        std::string text;
    };

    virtual ~ChatPanel() override;
    ChatPanel(uint64_t pTargetPlayerId, const std::string& pTargetUsername);

    void render() override;

    uint64_t getTargetPlayerId() const;
    const std::string& getTargetUsername() const;

    // Fed by WorldScene when a DirectMessageReceived push names this
    // conversation's target as the sender. Only flags the conversation as
    // unread if the window isn't currently open (isOpen, from Panel) - if
    // it's already open and visible, the new line just appears in it like
    // any other message, no separate notification needed.
    void appendIncoming(const std::string& pAuthorUsername, const std::string& pText);

    bool hasUnread() const;

    // Fully shows the conversation - opens it if closed, un-minimizes it if
    // minimized, and clears the unread flag. Used for every user-initiated
    // "open this chat" action (right-click -> Chat, clicking an unread
    // indicator) so none of them can leave it sitting minimized-but-open
    // with no visible change.
    void show();

private:
    uint64_t mTargetPlayerId;
    std::string mTargetUsername;

    std::vector<ChatLine> mHistory;
    char mInputBuffer[256];
    bool mHasUnread{false};

    // While minimized, render() only draws the title row (with a Restore
    // button) instead of the history/input - and switches windowFlags to
    // ImGuiWindowFlags_AlwaysAutoResize so the window itself shrinks to fit
    // that single row instead of keeping its expanded size. GuiLayer reads
    // windowFlags fresh every frame right before Begin(), so toggling it
    // here takes effect (with a harmless one-frame lag) without any change
    // needed on GuiLayer's side.
    bool mIsMinimized{false};

    void sendCurrentInput();
};

}

#endif
