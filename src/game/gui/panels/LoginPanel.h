#ifndef LAKOT_LOGINPANEL_H
#define LAKOT_LOGINPANEL_H

#include "../Panel.h"

namespace lakot
{

struct LoginPanel : public Panel
{
    virtual ~LoginPanel() override;
    LoginPanel();

    void render() override;
    void onLoginButtonClicked();

private:
    char mUsername[64];
    char mPassword[64];

    bool mIsLoggingIn;
    std::string mStatusMessage;
    bool mIsError;

    void onLoginResult(bool pIsSuccess, const std::string& pMessage);
};

}

#endif
