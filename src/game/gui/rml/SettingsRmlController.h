#ifndef LAKOT_SETTINGS_RML_CONTROLLER_H
#define LAKOT_SETTINGS_RML_CONTROLLER_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "RmlDocument.h"
#include "../../settings/DisplaySettings.h"
#include "../../settings/KeyBindings.h"

namespace lakot
{

class Localization;
class RmlUiLayer;

class SettingsRmlController
{
public:
    using ClosedCallback = std::function<void()>;

    SettingsRmlController(RmlUiLayer& pRmlUiLayer,
                          const Localization& pLocalization,
                          DisplaySettings& pDisplaySettings,
                          KeyBindings& pKeyBindings,
                          SDL_Window* pWindow);
    ~SettingsRmlController();

    SettingsRmlController(const SettingsRmlController&) = delete;
    SettingsRmlController& operator=(const SettingsRmlController&) = delete;

    void setClosedCallback(ClosedCallback pCallback);

    void open();
    void close();
    bool isOpen() const;

    bool isCapturingKey() const;
    bool handleKeyDown(const SDL_KeyboardEvent& pEvent);
    void cancelCapture();

private:
    struct OptionRow
    {
        int index = 0;
        Rml::String label;
        bool isSelected = false;
    };

    struct BindingRow
    {
        int index = 0;
        Rml::String label;
        Rml::String key;
        bool isWaiting = false;
    };

    RmlUiLayer& mRmlUiLayer;
    const Localization& mLocalization;
    DisplaySettings& mDisplaySettings;
    KeyBindings& mKeyBindings;
    SDL_Window* mWindow;

    RmlDocument mDocument;
    Rml::DataModelHandle mModelHandle;

    bool mIsOpen{false};
    bool mIsControlsTab{true};
    bool mIsVsyncEnabled{true};
    bool mIsCameraShakeEnabled{true};

    std::optional<KeyActionType> mCapturingAction;

    std::vector<DisplayMode> mDisplayModes;
    std::vector<OptionRow> mDisplayOptions;
    std::vector<OptionRow> mViewDistanceOptions;
    std::vector<BindingRow> mBindingRowsLeft;
    std::vector<BindingRow> mBindingRowsRight;

    ClosedCallback mClosedCallback;

    void refreshAll();
    void refreshDisplayOptions();
    void refreshViewDistanceOptions();
    void refreshBindings();

    void onSelectTab(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onBeginRebind(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onResetBindings(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSelectDisplay(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSelectViewDistance(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onToggleVsync(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onToggleCameraShake(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
};

}

#endif
