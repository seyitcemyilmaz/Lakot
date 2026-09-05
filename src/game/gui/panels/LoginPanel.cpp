#include "LoginPanel.h"

#include <imgui.h>

#include "../UiTheme.h"

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
    , mIsRegistering(false)
    , mIsRegisterMode(false)
    , mIsError(false)
    , mShowRegisterSuccessPopup(false)
{
    std::memset(mUsername, 0, sizeof(mUsername));
    std::memset(mPassword, 0, sizeof(mPassword));
    std::memset(mEmail, 0, sizeof(mEmail));

    Engine::getInstance().getNetworkManager().getAuthController().setLoginResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage, const AuthController::LoginSpawnState& pSpawnState)
    {
        this->onLoginResult(pIsSuccess, pMessage, pSpawnState);
    });

    Engine::getInstance().getNetworkManager().getAuthController().setRegisterResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage)
    {
        this->onRegisterResult(pIsSuccess, pMessage);
    });
}

void LoginPanel::render()
{
    ImVec2 viewportSize = ImGui::GetWindowSize();
    float boxHeight = mIsRegisterMode ? 460 : 380;

#if defined(LAKOT_DEV_QUICK_LOGIN)
    boxHeight += 130;
#endif

    // Never let the box grow taller than the visible window - otherwise the
    // outer full-screen panel itself grows a scrollbar instead of just this
    // box (any leftover overflow scrolls inside the box below instead).
    float maxBoxHeight = viewportSize.y - 40.0f;
    if (boxHeight > maxBoxHeight)
    {
        boxHeight = maxBoxHeight;
    }

    ImVec2 boxSize(400, boxHeight);

    ImGui::SetCursorPos(ImVec2((viewportSize.x - boxSize.x) * 0.5f, (viewportSize.y - boxSize.y) * 0.5f));

    // The login box is a deliberately elevated "card", distinct from the
    // plain (matching-background) child regions elsewhere - everywhere else
    // just inherits UiTheme's global ChildBg/ChildRounding defaults.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiTheme::kPanelBg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    ImGui::BeginChild("LoginBox", boxSize, true);

    float windowWidth = ImGui::GetContentRegionAvail().x;
    const char* titleText = "LAKOT ONLINE";
    float textWidth = ImGui::CalcTextSize(titleText).x;

    ImGui::Spacing();
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextColored(UiTheme::kAccent, "%s", titleText);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##username", "Username", mUsername, IM_ARRAYSIZE(mUsername));

    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##password", "Password", mPassword, IM_ARRAYSIZE(mPassword), ImGuiInputTextFlags_Password);

    if (mIsRegisterMode)
    {
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##email", "Email", mEmail, IM_ARRAYSIZE(mEmail));
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (!mStatusMessage.empty())
    {
        float msgWidth = ImGui::CalcTextSize(mStatusMessage.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - msgWidth) * 0.5f);
        if (mIsError) ImGui::TextColored(UiTheme::kDanger, "%s", mStatusMessage.c_str());
        else ImGui::TextColored(UiTheme::kInfo, "%s", mStatusMessage.c_str());
    }
    else
    {
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
    }

    ImGui::Spacing();
    ImGui::Spacing();

    bool tIsBusy = mIsLoggingIn || mIsRegistering;

    float bottomBlockHeight = 85.0f;

#if defined(LAKOT_DEV_QUICK_LOGIN)
    bottomBlockHeight += 110.0f;
#endif

    float availY = ImGui::GetContentRegionAvail().y;
    if (availY > bottomBlockHeight) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availY - bottomBlockHeight);


    if (tIsBusy)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, UiTheme::kAccent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UiTheme::kAccentHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, UiTheme::kAccentActive);
    }

    const char* actionButtonText = mIsRegisterMode ? "REGISTER" : "LOG IN";

    if (ImGui::Button(actionButtonText, ImVec2(-1, 45)))
    {
        if (!tIsBusy)
        {
            if (mIsRegisterMode)
            {
                onRegisterButtonClicked();
            }
            else
            {
                onLoginButtonClicked();
            }
        }
    }

    ImGui::PopStyleColor(3);

    if (!tIsBusy && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
    {
        if (mIsRegisterMode)
        {
            onRegisterButtonClicked();
        }
        else
        {
            onLoginButtonClicked();
        }
    }

    ImGui::Spacing();

    const char* toggleText = mIsRegisterMode ? "Already have an account? Log in" : "Don't have an account? Register";
    float toggleWidth = ImGui::CalcTextSize(toggleText).x;
    ImGui::SetCursorPosX((windowWidth - toggleWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, UiTheme::kInfo);

    if (ImGui::Button(toggleText))
    {
        if (!tIsBusy)
        {
            onToggleModeClicked();
        }
    }

    ImGui::PopStyleColor(4);

#if defined(LAKOT_DEV_QUICK_LOGIN)
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Dev quick login:");

    if (ImGui::Button("lakot1 giris yap", ImVec2(-1, 0)))
    {
        quickLogin("lakot1", "lakot1");
    }

    if (ImGui::Button("lakot2 giris yap", ImVec2(-1, 0)))
    {
        quickLogin("lakot2", "lakot2");
    }
#endif

    ImGui::EndChild();

    ImGui::PopStyleVar();  // WindowPadding
    ImGui::PopStyleColor(); // ChildBg

    if (mShowRegisterSuccessPopup)
    {
        ImGui::OpenPopup("Registration Successful");
        mShowRegisterSuccessPopup = false;
    }

    if (ImGui::BeginPopupModal("Registration Successful", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Your account has been created. You can now log in.");
        ImGui::Spacing();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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

    Engine::getInstance().getNetworkManager().getAuthController().sendLoginRequest(mUsername, mPassword);
}

void LoginPanel::onRegisterButtonClicked()
{
    if (strlen(mUsername) < 3 || strlen(mPassword) < 3 || strlen(mEmail) == 0)
    {
        mStatusMessage = "Please fill in all fields.";
        mIsError = true;
        return;
    }

    mIsRegistering = true;
    mStatusMessage = "Registering...";
    mIsError = false;

    Engine::getInstance().getNetworkManager().getAuthController().sendRegisterAccount(mUsername, mPassword, mEmail);
}

void LoginPanel::onLoginResult(bool pIsSuccess, const std::string& pMessage, const AuthController::LoginSpawnState& pSpawnState)
{
    mIsLoggingIn = false;
    mStatusMessage = pMessage;
    mIsError = !pIsSuccess;

    if (pIsSuccess)
    {
        WorldScene::InitialSpawnState tSpawnState{ pSpawnState.mapId, pSpawnState.x, pSpawnState.y, pSpawnState.z, pSpawnState.yaw };
        Engine::getInstance().getSceneManager().setNextScene(std::make_unique<WorldScene>(Engine::getInstance(), tSpawnState, std::string(mUsername)));
    }
}

void LoginPanel::onRegisterResult(bool pIsSuccess, const std::string& pMessage)
{
    mIsRegistering = false;

    if (pIsSuccess)
    {
        mIsError = false;
        mStatusMessage = "";
        mIsRegisterMode = false;
        mShowRegisterSuccessPopup = true;
    }
    else
    {
        mIsError = true;
        mStatusMessage = pMessage;
    }
}

void LoginPanel::onToggleModeClicked()
{
    mIsRegisterMode = !mIsRegisterMode;
    mIsError = false;
    mStatusMessage = "";

    std::memset(mEmail, 0, sizeof(mEmail));
}

#if defined(LAKOT_DEV_QUICK_LOGIN)
void LoginPanel::quickLogin(const char* pUsername, const char* pPassword)
{
    std::strncpy(mUsername, pUsername, sizeof(mUsername) - 1);
    mUsername[sizeof(mUsername) - 1] = '\0';

    std::strncpy(mPassword, pPassword, sizeof(mPassword) - 1);
    mPassword[sizeof(mPassword) - 1] = '\0';

    onLoginButtonClicked();
}
#endif
