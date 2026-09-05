#include "ChatPanel.h"

#include <cstring>

#include <imgui.h>

#include "../UiTheme.h"

#include "../../Engine.h"

using namespace lakot;

ChatPanel::~ChatPanel()
{

}

ChatPanel::ChatPanel(uint64_t pTargetPlayerId, const std::string& pTargetUsername)
    : Panel("Chat - " + pTargetUsername, ImGuiWindowFlags_None, false, false)
    , mTargetPlayerId(pTargetPlayerId)
    , mTargetUsername(pTargetUsername)
{
    std::memset(mInputBuffer, 0, sizeof(mInputBuffer));
}

uint64_t ChatPanel::getTargetPlayerId() const
{
    return mTargetPlayerId;
}

const std::string& ChatPanel::getTargetUsername() const
{
    return mTargetUsername;
}

void ChatPanel::appendIncoming(const std::string& pAuthorUsername, const std::string& pText)
{
    mHistory.push_back(ChatLine{ false, pAuthorUsername, pText });

    // Flag unread unless the conversation is both open AND expanded - a
    // minimized window only shows its title row, so a message arriving
    // then is just as unseen as one arriving while fully closed.
    if (!isOpen || mIsMinimized)
    {
        mHasUnread = true;
    }
}

bool ChatPanel::hasUnread() const
{
    return mHasUnread;
}

void ChatPanel::show()
{
    isOpen = true;
    mIsMinimized = false;
    mHasUnread = false;
}

void ChatPanel::render()
{
    // A small, explicit minimize/restore row - "_" collapses the window
    // down to just this row (with an unread badge if a message arrives
    // while minimized), "Restore" brings the full conversation back.
    if (mIsMinimized)
    {
        std::string tLabel = mTargetUsername;
        if (mHasUnread)
        {
            tLabel += " (new)";
        }
        ImGui::TextUnformatted(tLabel.c_str());
        ImGui::SameLine();

        if (ImGui::SmallButton("Restore"))
        {
            mIsMinimized = false;
        }

        windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
        return;
    }

    // Fully expanded and visible - whatever prompted the unread flag has
    // now been seen.
    mHasUnread = false;

    if (ImGui::SmallButton("_"))
    {
        mIsMinimized = true;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("%s", mTargetUsername.c_str());

    windowFlags = ImGuiWindowFlags_None;

    ImGui::SetWindowSize(ImVec2(280, 340), ImGuiCond_FirstUseEver);

    ImVec2 tHistorySize(-1.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing());

    ImGui::BeginChild("ChatHistory", tHistorySize, true, ImGuiWindowFlags_HorizontalScrollbar);

    for (const ChatLine& tLine : mHistory)
    {
        ImVec4 tColor = tLine.isLocal ? UiTheme::kInfo : UiTheme::kWarn;
        ImGui::TextColored(tColor, "%s:", tLine.author.c_str());
        ImGui::SameLine();
        ImGui::TextWrapped("%s", tLine.text.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);

    bool tEnterPressed = ImGui::InputText("##ChatInput", mInputBuffer, IM_ARRAYSIZE(mInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    bool tSendClicked = ImGui::Button("Send", ImVec2(-1, 0));

    if ((tEnterPressed || tSendClicked) && std::strlen(mInputBuffer) > 0)
    {
        sendCurrentInput();
    }
}

void ChatPanel::sendCurrentInput()
{
    std::string tText(mInputBuffer);

    Engine::getInstance().getNetworkManager().getChatController().sendDirectMessage(mTargetUsername, tText);

    // Appended optimistically - the server only acks success/failure, it
    // doesn't echo the text back, so the sender's own window would
    // otherwise never show what they just sent.
    mHistory.push_back(ChatLine{ true, "You", tText });

    std::memset(mInputBuffer, 0, sizeof(mInputBuffer));
}
