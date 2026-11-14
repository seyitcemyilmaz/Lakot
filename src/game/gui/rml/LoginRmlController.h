#ifndef LAKOT_LOGINRMLCONTROLLER_H
#define LAKOT_LOGINRMLCONTROLLER_H

#include <functional>
#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "RmlDocument.h"
#include "../../network/controller/AuthController.h"

namespace lakot
{

class RmlUiLayer;
class Localization;

// RmlUi replacement for the ImGui LoginPanel - loads ui/login.rml and binds
// a data model ("login") the document reads/writes via data-value/data-if,
// with its buttons wired through data-event-click straight to bound member
// functions instead of a per-button Rml::EventListener. Makes the same
// AuthController calls the old panel made. Owned by LoginScene for the
// lifetime of the login screen.
class LoginRmlController
{
public:
    using LoginSucceededCallback = std::function<void()>;

    LoginRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, AuthController& pAuthController, LoginSucceededCallback pOnLoginSucceeded, const std::string& pNotice);
    ~LoginRmlController();

    LoginRmlController(const LoginRmlController&) = delete;
    LoginRmlController& operator=(const LoginRmlController&) = delete;

private:
    RmlUiLayer& mRmlUiLayer;
    const Localization& mLocalization;
    AuthController& mAuthController;
    LoginSucceededCallback mOnLoginSucceeded;

    RmlDocument mDocument;
    Rml::DataModelHandle mModelHandle;

    Rml::String mUsername;
    Rml::String mPassword;
    Rml::String mEmail;
    Rml::String mStatusMessage;
    bool mIsError{false};
    bool mIsRegisterMode{false};
    bool mIsBusy{false};
    bool mIsDevQuickLoginEnabled{false};

    void onLoginButtonClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onRegisterButtonClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onToggleModeClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onQuickLoginClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // Same fix as ChatRmlController::onInputKeyDown - RmlUi's SDL backend
    // turns Enter into a literal '\n' text-input character rather than a
    // submit action, and these fields aren't wrapped in a <form>, so without
    // this Enter does nothing in either field. Submits whichever action is
    // currently active (register vs. login), matching what the visible
    // button would do.
    void onInputKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void doLogin();
    void doRegister();

    void onLoginResult(bool pIsSuccess, const std::string& pMessage);
    void onRegisterResult(bool pIsSuccess, const std::string& pMessage);

    void setStatus(const std::string& pMessage, bool pIsError);
    void dirtyAll();
};

}

#endif
