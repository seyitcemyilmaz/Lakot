#include "SystemMenuRmlController.h"

#include <SDL3/SDL_events.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

SystemMenuRmlController::SystemMenuRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("system_menu");

    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tOption = tConstructor.RegisterStruct<DisplayOptionRow>();
        tOption.RegisterMember("index", &DisplayOptionRow::index);
        tOption.RegisterMember("label", &DisplayOptionRow::label);
        tOption.RegisterMember("is_selected", &DisplayOptionRow::isSelected);
        tConstructor.RegisterArray<std::vector<DisplayOptionRow>>();

        sTypesRegistered = true;
    }

    tConstructor.Bind("is_settings_open", &mIsSettingsOpen);
    tConstructor.Bind("display_options", &mDisplayOptions);

    tConstructor.BindEventCallback("resume", &SystemMenuRmlController::onResumeClicked, this);
    tConstructor.BindEventCallback("open_settings", &SystemMenuRmlController::onOpenSettingsClicked, this);
    tConstructor.BindEventCallback("close_settings", &SystemMenuRmlController::onCloseSettingsClicked, this);
    tConstructor.BindEventCallback("select_display", &SystemMenuRmlController::onDisplaySelected, this);
    tConstructor.BindEventCallback("logout", &SystemMenuRmlController::onLogoutClicked, this);
    tConstructor.BindEventCallback("quit", &SystemMenuRmlController::onQuitClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/systemmenu.rml");
}

SystemMenuRmlController::~SystemMenuRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("system_menu");
    }
}

void SystemMenuRmlController::setLogoutCallback(LogoutCallback pCallback)
{
    mLogoutCallback = std::move(pCallback);
}

void SystemMenuRmlController::setDisplaySelectCallback(DisplaySelectCallback pCallback)
{
    mDisplaySelectCallback = std::move(pCallback);
}

void SystemMenuRmlController::setDisplayOptions(const std::vector<std::string>& pLabels, size_t pSelected)
{
    mDisplayOptions.clear();

    for (size_t tIndex = 0; tIndex < pLabels.size(); ++tIndex)
    {
        mDisplayOptions.push_back({ static_cast<int>(tIndex), pLabels[tIndex], tIndex == pSelected });
    }

    mModelHandle.DirtyVariable("display_options");
}

bool SystemMenuRmlController::isSettingsOpen() const
{
    return mIsOpen && mIsSettingsOpen;
}

void SystemMenuRmlController::closeSettings()
{
    mIsSettingsOpen = false;
    mModelHandle.DirtyVariable("is_settings_open");
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
    closeSettings();

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
    mIsSettingsOpen = true;
    mModelHandle.DirtyVariable("is_settings_open");
}

void SystemMenuRmlController::onCloseSettingsClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    closeSettings();
}

void SystemMenuRmlController::onDisplaySelected(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || !mDisplaySelectCallback)
    {
        return;
    }

    int tIndex = pArguments[0].Get<int>(-1);

    if (tIndex >= 0 && static_cast<size_t>(tIndex) < mDisplayOptions.size())
    {
        mDisplaySelectCallback(static_cast<size_t>(tIndex));
    }
}
