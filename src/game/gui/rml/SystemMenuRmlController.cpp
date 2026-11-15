#include "SystemMenuRmlController.h"

#include <SDL3/SDL_events.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

SystemMenuRmlController::SystemMenuRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("system_menu");

    tConstructor.BindEventCallback("resume", &SystemMenuRmlController::onResumeClicked, this);
    tConstructor.BindEventCallback("open_settings", &SystemMenuRmlController::onOpenSettingsClicked, this);
    tConstructor.BindEventCallback("logout", &SystemMenuRmlController::onLogoutClicked, this);
    tConstructor.BindEventCallback("quit", &SystemMenuRmlController::onQuitClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/systemmenu.rml");
}

SystemMenuRmlController::~SystemMenuRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("system_menu");
    }
}

void SystemMenuRmlController::setLogoutCallback(LogoutCallback pCallback)
{
    mLogoutCallback = std::move(pCallback);
}

void SystemMenuRmlController::setSettingsCallback(SettingsCallback pCallback)
{
    mSettingsCallback = std::move(pCallback);
}

void SystemMenuRmlController::open()
{
    if (mIsOpen)
    {
        return;
    }

    mIsOpen = true;

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void SystemMenuRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool SystemMenuRmlController::isOpen() const
{
    return mIsOpen;
}

void SystemMenuRmlController::onResumeClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void SystemMenuRmlController::onLogoutClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();

    if (mLogoutCallback)
    {
        mLogoutCallback();
    }
}

void SystemMenuRmlController::onQuitClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    SDL_Event tEvent{};
    tEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&tEvent);
}

void SystemMenuRmlController::onOpenSettingsClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();

    if (mSettingsCallback)
    {
        mSettingsCallback();
    }
}
