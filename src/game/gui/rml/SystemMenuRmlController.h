#ifndef LAKOT_SYSTEM_MENU_RML_CONTROLLER_H
#define LAKOT_SYSTEM_MENU_RML_CONTROLLER_H

#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class RmlUiLayer;

// The in-world Escape menu.
class SystemMenuRmlController
{
public:
    using LogoutCallback = std::function<void()>;
    using DisplaySelectCallback = std::function<void(size_t pIndex)>;

    explicit SystemMenuRmlController(RmlUiLayer& pRmlUiLayer);
    ~SystemMenuRmlController();

    SystemMenuRmlController(const SystemMenuRmlController&) = delete;
    SystemMenuRmlController& operator=(const SystemMenuRmlController&) = delete;

    void setLogoutCallback(LogoutCallback pCallback);
    void setDisplaySelectCallback(DisplaySelectCallback pCallback);

    void setDisplayOptions(const std::vector<std::string>& pLabels, size_t pSelected);

    void open();
    void close();
    bool isOpen() const;

    bool isSettingsOpen() const;
    void closeSettings();

private:
    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    struct DisplayOptionRow
    {
        int index = 0;
        Rml::String label;
        bool isSelected = false;
    };

    bool mIsOpen{false};
    bool mIsSettingsOpen{false};

    std::vector<DisplayOptionRow> mDisplayOptions;

    LogoutCallback mLogoutCallback;
    DisplaySelectCallback mDisplaySelectCallback;

    void onResumeClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onLogoutClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onQuitClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onOpenSettingsClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCloseSettingsClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDisplaySelected(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
};

}

#endif
