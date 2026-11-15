#include "LoginRmlController.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Input.h>

#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

LoginRmlController::LoginRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, AuthController& pAuthController, LoginSucceededCallback pLoginSucceededCallback, const std::string& pNotice)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mAuthController(pAuthController)
    , mLoginSucceededCallback(std::move(pLoginSucceededCallback))
    , mStatusMessage(pNotice)
    , mIsError(!pNotice.empty())
{
#if defined(LAKOT_DEV_QUICK_LOGIN)
    mIsDevQuickLoginEnabled = true;
#endif

    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("login");

    tConstructor.Bind("username", &mUsername);
    tConstructor.Bind("password", &mPassword);
    tConstructor.Bind("email", &mEmail);
    tConstructor.Bind("status_message", &mStatusMessage);
    tConstructor.Bind("is_error", &mIsError);
    tConstructor.Bind("is_register_mode", &mIsRegisterMode);
    tConstructor.Bind("is_busy", &mIsBusy);
    tConstructor.Bind("is_dev_quick_login_enabled", &mIsDevQuickLoginEnabled);

    tConstructor.BindEventCallback("login", &LoginRmlController::onLoginButtonClicked, this);
    tConstructor.BindEventCallback("register", &LoginRmlController::onRegisterButtonClicked, this);
    tConstructor.BindEventCallback("toggle_mode", &LoginRmlController::onToggleModeClicked, this);
    tConstructor.BindEventCallback("quick_login", &LoginRmlController::onQuickLoginClicked, this);
    tConstructor.BindEventCallback("input_keydown", &LoginRmlController::onInputKeyDown, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/login.rml");

    if (mDocument)
    {
        mDocument->Show();
    }

    mAuthController.setLoginResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage)
    {
        this->onLoginResult(pIsSuccess, pMessage);
    });

    mAuthController.setRegisterResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage)
    {
        this->onRegisterResult(pIsSuccess, pMessage);
    });
}

LoginRmlController::~LoginRmlController()
{
    mAuthController.setLoginResultCallback(nullptr);
    mAuthController.setRegisterResultCallback(nullptr);

    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("login");
    }
}

void LoginRmlController::onLoginButtonClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (!mIsBusy)
    {
        doLogin();
    }
}

void LoginRmlController::onRegisterButtonClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (!mIsBusy)
    {
        doRegister();
    }
}

void LoginRmlController::onInputKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if ((tKeyIdentifier != Rml::Input::KI_RETURN && tKeyIdentifier != Rml::Input::KI_NUMPADENTER) || mIsBusy)
    {
        return;
    }

    if (mIsRegisterMode)
    {
        doRegister();
    }
    else
    {
        doLogin();
    }
}

void LoginRmlController::onToggleModeClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mIsBusy)
    {
        return;
    }

    mIsRegisterMode = !mIsRegisterMode;
    mEmail.clear();
    setStatus("", false);
}

void LoginRmlController::onQuickLoginClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (mIsBusy || pArguments.size() < 2)
    {
        return;
    }

    mUsername = pArguments[0].Get<Rml::String>();
    mPassword = pArguments[1].Get<Rml::String>();
    dirtyAll();

    doLogin();
}

void LoginRmlController::doLogin()
{
    if (mUsername.size() < 3 || mPassword.size() < 3)
    {
        setStatus(mLocalization.text("login.error.username_short"), true);
        return;
    }

    mIsBusy = true;
    setStatus(mLocalization.text("login.status.connecting"), false);

    mAuthController.sendLoginRequest(mUsername, mPassword);
}

void LoginRmlController::doRegister()
{
    if (mUsername.size() < 3 || mPassword.size() < 3 || mEmail.empty())
    {
        setStatus(mLocalization.text("login.error.fill_all"), true);
        return;
    }

    mIsBusy = true;
    setStatus(mLocalization.text("login.status.registering"), false);

    mAuthController.sendRegisterAccount(mUsername, mPassword, mEmail);
}

void LoginRmlController::onLoginResult(bool pIsSuccess, const std::string& pMessage)
{
    mIsBusy = pIsSuccess;
    setStatus(pMessage, !pIsSuccess);

    if (pIsSuccess)
    {
        if (mLoginSucceededCallback)
        {
            mLoginSucceededCallback();
        }
    }
    else
    {
        dirtyAll();
    }
}

void LoginRmlController::onRegisterResult(bool pIsSuccess, const std::string& pMessage)
{
    mIsBusy = false;

    if (pIsSuccess)
    {
        mIsRegisterMode = false;
        // No separate modal document for this in the RmlUi version (the
        // ImGui LoginPanel's "Registration Successful" popup conveyed
        // nothing the status line below doesn't already say) - status text
        // carries the confirmation instead.
        setStatus(mLocalization.text("auth.register.created"), false);
    }
    else
    {
        setStatus(pMessage, true);
    }
}

void LoginRmlController::setStatus(const std::string& pMessage, bool pIsError)
{
    mStatusMessage = pMessage;
    mIsError = pIsError;
    dirtyAll();
}

void LoginRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();
}
