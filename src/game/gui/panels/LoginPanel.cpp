#include "LoginPanel.h"

#include <imgui.h>

#include "../../Engine.h"

#include "../../scene/WorldScene.h"

using namespace lakot;

LoginPanel::~LoginPanel()
{

}

LoginPanel::LoginPanel()
    : Panel("Lakot Online"
    , ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
    , false
    , true)
    , mIsLoggingIn(false)
    , mIsError(false)
{
    std::memset(mUsername, 0, sizeof(mUsername));
    std::memset(mPassword, 0, sizeof(mPassword));

    Engine::getInstance().getNetworkManager().getAuthController().setLoginResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage)
    {
        this->onLoginResult(pIsSuccess, pMessage);
    });
}

void LoginPanel::render()
{
    ImVec2 viewportSize = ImGui::GetWindowSize();
    ImVec2 boxSize(400, 380);

    ImGui::SetCursorPos(ImVec2((viewportSize.x - boxSize.x) * 0.5f, (viewportSize.y - boxSize.y) * 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    ImGui::BeginChild("LoginBox", boxSize, true);

    float windowWidth = ImGui::GetContentRegionAvail().x;
    const char* titleText = "LAKOT ONLINE";
    float textWidth = ImGui::CalcTextSize(titleText).x;

    ImGui::Spacing();
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", titleText);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##username", "Username", mUsername, IM_ARRAYSIZE(mUsername));

    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##password", "Password", mPassword, IM_ARRAYSIZE(mPassword), ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    ImGui::Spacing();

    if (!mStatusMessage.empty())
    {
        float msgWidth = ImGui::CalcTextSize(mStatusMessage.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - msgWidth) * 0.5f);
        if (mIsError) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", mStatusMessage.c_str());
        else ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", mStatusMessage.c_str());
    }
    else
    {
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
    }

    ImGui::Spacing();
    ImGui::Spacing();

    float availY = ImGui::GetContentRegionAvail().y;
    if (availY > 50) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availY - 50);


    if (mIsLoggingIn)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 0.1f, 1.0f));
    }

    if (ImGui::Button("LOG IN", ImVec2(-1, 45)))
    {
        if (!mIsLoggingIn)
        {
            onLoginButtonClicked();
        }
    }

    ImGui::PopStyleColor(3);

    if (!mIsLoggingIn && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
    {
        onLoginButtonClicked();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void LoginPanel::onLoginButtonClicked()
{
    if (strlen(mUsername) < 3 || strlen(mPassword) < 3)
    {
        mStatusMessage = "Username too short!";
        mIsError = true;
        return;
    }

    mIsLoggingIn = true;
    mStatusMessage = "Connecting...";
    mIsError = false;

    Engine::getInstance().getNetworkManager().getAuthController().login(mUsername, mPassword);
}

void LoginPanel::onLoginResult(bool pIsSuccess, const std::string& pMessage)
{
    mIsLoggingIn = false;
    mStatusMessage = pMessage;
    mIsError = !pIsSuccess;

    if (pIsSuccess)
    {
        Engine::getInstance().getSceneManager().setNextScene(std::make_unique<WorldScene>(Engine::getInstance()));
    }
}
