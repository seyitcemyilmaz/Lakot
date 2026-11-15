#ifndef LAKOT_SYSTEM_MENU_RML_CONTROLLER_H
#define LAKOT_SYSTEM_MENU_RML_CONTROLLER_H

#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "RmlDocument.h"

namespace lakot
{

class RmlUiLayer;

// The in-world Escape menu.
class SystemMenuRmlController
{
public:
    using LogoutCallback = std::function<void()>;
    using SettingsCallback = std::function<void()>;

    explicit SystemMenuRmlController(RmlUiLayer& pRmlUiLayer);
    ~SystemMenuRmlController();

    SystemMenuRmlController(const SystemMenuRmlController&) = delete;
    SystemMenuRmlController& operator=(const SystemMenuRmlController&) = delete;

    void setLogoutCallback(LogoutCallback pCallback);
    void setSettingsCallback(SettingsCallback pCallback);

    void open();
    void close();
    bool isOpen() const;

private:
    RmlUiLayer& mRmlUiLayer;

    RmlDocument mDocument;
    Rml::DataModelHandle mModelHandle;

    bool mIsOpen{false};

    LogoutCallback mLogoutCallback;
    SettingsCallback mSettingsCallback;

    void onResumeClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onLogoutClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onQuitClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onOpenSettingsClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
};

}

#endif
