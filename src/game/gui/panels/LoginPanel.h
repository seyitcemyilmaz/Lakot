#ifndef LAKOT_LOGINPANEL_H
#define LAKOT_LOGINPANEL_H

#include "../Panel.h"
#include "../../network/controller/AuthController.h"

// Local-testing quick-login buttons ("lakot1 giris yap" / "lakot2 giris
// yap") on the login screen, so you don't have to type credentials every
// run. Comment out before release - mirrors the LAKOT_DEV_MODE toggle in
// Engine.cpp.
#define LAKOT_DEV_QUICK_LOGIN

namespace lakot
{

struct LoginPanel : public Panel
{
    virtual ~LoginPanel() override;
    LoginPanel();

    void render() override;
    void onLoginButtonClicked();
    void onRegisterButtonClicked();

private:
    char mUsername[64];
    char mPassword[64];
    char mEmail[128];

    bool mIsLoggingIn;
    bool mIsRegistering;
    bool mIsRegisterMode;
    bool mIsError;
    bool mShowRegisterSuccessPopup;
    std::string mStatusMessage;

    void onLoginResult(bool pIsSuccess, const std::string& pMessage, const AuthController::LoginSpawnState& pSpawnState);
    void onRegisterResult(bool pIsSuccess, const std::string& pMessage);
    void onToggleModeClicked();

#if defined(LAKOT_DEV_QUICK_LOGIN)
    void quickLogin(const char* pUsername, const char* pPassword);
#endif
};

}

#endif
