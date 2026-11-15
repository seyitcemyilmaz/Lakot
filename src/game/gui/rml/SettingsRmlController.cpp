#include "SettingsRmlController.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

namespace
{
    int getIntArgument(const Rml::VariantList& pArguments)
    {
        return pArguments.empty() ? -1 : pArguments[0].Get<int>(-1);
    }

    std::string getActionLabel(const Localization& pLocalization, KeyActionType pAction)
    {
        switch (pAction)
        {
            case KeyActionType::eMoveForward: return pLocalization.text("settings.action.move_forward");
            case KeyActionType::eMoveBackward: return pLocalization.text("settings.action.move_backward");
            case KeyActionType::eMoveLeft: return pLocalization.text("settings.action.move_left");
            case KeyActionType::eMoveRight: return pLocalization.text("settings.action.move_right");
            case KeyActionType::eAttack: return pLocalization.text("settings.action.attack");
            case KeyActionType::eAutoPickup: return pLocalization.text("settings.action.auto_pickup");
            case KeyActionType::eInventory: return pLocalization.text("settings.action.inventory");
            case KeyActionType::eWorldMap: return pLocalization.text("settings.action.world_map");
            case KeyActionType::eCharacterSheet: return pLocalization.text("settings.action.character_sheet");
            case KeyActionType::eSkillWindow: return pLocalization.text("settings.action.skill_window");
            case KeyActionType::eSkillSlot1: return pLocalization.text("settings.action.skill_slot_1");
            case KeyActionType::eSkillSlot2: return pLocalization.text("settings.action.skill_slot_2");
            case KeyActionType::eSkillSlot3: return pLocalization.text("settings.action.skill_slot_3");
            case KeyActionType::eSkillSlot4: return pLocalization.text("settings.action.skill_slot_4");
            case KeyActionType::eSkillSlot5: return pLocalization.text("settings.action.skill_slot_5");
            case KeyActionType::eSkillSlotF1: return pLocalization.text("settings.action.skill_slot_f1");
            case KeyActionType::eSkillSlotF2: return pLocalization.text("settings.action.skill_slot_f2");
            case KeyActionType::eSkillSlotF3: return pLocalization.text("settings.action.skill_slot_f3");
            case KeyActionType::eSkillSlotF4: return pLocalization.text("settings.action.skill_slot_f4");
            case KeyActionType::eSkillSlotF5: return pLocalization.text("settings.action.skill_slot_f5");
            default: return std::string();
        }
    }
}

SettingsRmlController::SettingsRmlController(RmlUiLayer& pRmlUiLayer,
                                             const Localization& pLocalization,
                                             DisplaySettings& pDisplaySettings,
                                             KeyBindings& pKeyBindings,
                                             SDL_Window* pWindow)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mDisplaySettings(pDisplaySettings)
    , mKeyBindings(pKeyBindings)
    , mWindow(pWindow)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("settings");

    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tOption = tConstructor.RegisterStruct<OptionRow>();
        tOption.RegisterMember("index", &OptionRow::index);
        tOption.RegisterMember("label", &OptionRow::label);
        tOption.RegisterMember("is_selected", &OptionRow::isSelected);
        tConstructor.RegisterArray<std::vector<OptionRow>>();

        auto tBinding = tConstructor.RegisterStruct<BindingRow>();
        tBinding.RegisterMember("index", &BindingRow::index);
        tBinding.RegisterMember("label", &BindingRow::label);
        tBinding.RegisterMember("key", &BindingRow::key);
        tBinding.RegisterMember("is_waiting", &BindingRow::isWaiting);
        tConstructor.RegisterArray<std::vector<BindingRow>>();

        sTypesRegistered = true;
    }

    tConstructor.Bind("is_controls_tab", &mIsControlsTab);
    tConstructor.Bind("is_vsync_enabled", &mIsVsyncEnabled);
    tConstructor.Bind("is_camera_shake_enabled", &mIsCameraShakeEnabled);
    tConstructor.Bind("display_options", &mDisplayOptions);
    tConstructor.Bind("view_distance_options", &mViewDistanceOptions);
    tConstructor.Bind("binding_rows_left", &mBindingRowsLeft);
    tConstructor.Bind("binding_rows_right", &mBindingRowsRight);

    tConstructor.BindEventCallback("select_tab", &SettingsRmlController::onSelectTab, this);
    tConstructor.BindEventCallback("begin_rebind", &SettingsRmlController::onBeginRebind, this);
    tConstructor.BindEventCallback("reset_bindings", &SettingsRmlController::onResetBindings, this);
    tConstructor.BindEventCallback("select_display", &SettingsRmlController::onSelectDisplay, this);
    tConstructor.BindEventCallback("select_view_distance", &SettingsRmlController::onSelectViewDistance, this);
    tConstructor.BindEventCallback("toggle_vsync", &SettingsRmlController::onToggleVsync, this);
    tConstructor.BindEventCallback("toggle_camera_shake", &SettingsRmlController::onToggleCameraShake, this);
    tConstructor.BindEventCallback("close", &SettingsRmlController::onCloseClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/settings.rml");
}

SettingsRmlController::~SettingsRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("settings");
    }
}

void SettingsRmlController::setClosedCallback(ClosedCallback pCallback)
{
    mClosedCallback = std::move(pCallback);
}

void SettingsRmlController::open()
{
    if (mIsOpen)
    {
        return;
    }

    mIsOpen = true;
    mCapturingAction.reset();
    refreshAll();

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void SettingsRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;
    mCapturingAction.reset();

    if (mDocument)
    {
        mDocument->Hide();
    }

    if (mClosedCallback)
    {
        mClosedCallback();
    }
}

bool SettingsRmlController::isOpen() const
{
    return mIsOpen;
}

bool SettingsRmlController::isCapturingKey() const
{
    return mIsOpen && mCapturingAction.has_value();
}

bool SettingsRmlController::handleKeyDown(const SDL_KeyboardEvent& pEvent)
{
    if (!isCapturingKey())
    {
        return false;
    }

    if (pEvent.repeat)
    {
        return true;
    }

    if (pEvent.scancode == SDL_SCANCODE_ESCAPE)
    {
        cancelCapture();
        return true;
    }

    if (mKeyBindings.rebind(*mCapturingAction, pEvent.scancode))
    {
        mCapturingAction.reset();
        refreshBindings();
    }

    return true;
}

void SettingsRmlController::cancelCapture()
{
    mCapturingAction.reset();
    refreshBindings();
}

void SettingsRmlController::refreshAll()
{
    mIsVsyncEnabled = mDisplaySettings.isVsyncEnabled();
    mIsCameraShakeEnabled = mDisplaySettings.isCameraShakeEnabled();
    mModelHandle.DirtyVariable("is_vsync_enabled");
    mModelHandle.DirtyVariable("is_camera_shake_enabled");

    refreshDisplayOptions();
    refreshViewDistanceOptions();
    refreshBindings();
}

void SettingsRmlController::refreshDisplayOptions()
{
    mDisplayModes = DisplaySettings::getAvailableModes(mWindow);
    mDisplayOptions.clear();

    for (size_t tIndex = 0; tIndex < mDisplayModes.size(); ++tIndex)
    {
        mDisplayOptions.push_back({ static_cast<int>(tIndex), mDisplayModes[tIndex].getLabel(mLocalization), mDisplayModes[tIndex] == mDisplaySettings.getMode() });
    }

    mModelHandle.DirtyVariable("display_options");
}

void SettingsRmlController::refreshViewDistanceOptions()
{
    mViewDistanceOptions.clear();

    for (size_t tLevel = 0; tLevel < DisplaySettings::getViewDistanceLevelCount(); ++tLevel)
    {
        int tPercent = static_cast<int>(DisplaySettings::getViewDistanceScale(tLevel) * 100.0f + 0.5f);
        mViewDistanceOptions.push_back({ static_cast<int>(tLevel), std::to_string(tPercent) + "%", tLevel == mDisplaySettings.getViewDistanceLevel() });
    }

    mModelHandle.DirtyVariable("view_distance_options");
}

void SettingsRmlController::refreshBindings()
{
    mBindingRowsLeft.clear();
    mBindingRowsRight.clear();

    constexpr size_t kActionCount = static_cast<size_t>(KeyActionType::eCount);

    for (size_t tIndex = 0; tIndex < kActionCount; ++tIndex)
    {
        KeyActionType tAction = static_cast<KeyActionType>(tIndex);
        bool tIsWaiting = mCapturingAction && *mCapturingAction == tAction;

        std::string tKey = tIsWaiting ? mLocalization.text("settings.press_key") : KeyBindings::getKeyLabel(mKeyBindings.getKey(tAction));

        std::vector<BindingRow>& tColumn = tIndex < (kActionCount + 1) / 2 ? mBindingRowsLeft : mBindingRowsRight;
        tColumn.push_back({ static_cast<int>(tIndex), getActionLabel(mLocalization, tAction), tKey, tIsWaiting });
    }

    mModelHandle.DirtyVariable("binding_rows_left");
    mModelHandle.DirtyVariable("binding_rows_right");
}

void SettingsRmlController::onSelectTab(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    int tTab = getIntArgument(pArguments);

    if (tTab < 0)
    {
        return;
    }

    mIsControlsTab = (tTab == 0);
    mModelHandle.DirtyVariable("is_controls_tab");

    if (!mIsControlsTab)
    {
        cancelCapture();
    }
}

void SettingsRmlController::onBeginRebind(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    int tIndex = getIntArgument(pArguments);

    if (tIndex < 0 || tIndex >= static_cast<int>(KeyActionType::eCount))
    {
        return;
    }

    mCapturingAction = static_cast<KeyActionType>(tIndex);
    refreshBindings();
}

void SettingsRmlController::onResetBindings(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mCapturingAction.reset();
    mKeyBindings.resetToDefaults();
    mKeyBindings.save();
    refreshBindings();
}

void SettingsRmlController::onSelectDisplay(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    int tIndex = getIntArgument(pArguments);

    if (tIndex >= 0 && static_cast<size_t>(tIndex) < mDisplayModes.size())
    {
        mDisplaySettings.change(mWindow, mDisplayModes[static_cast<size_t>(tIndex)]);
        refreshDisplayOptions();
    }
}

void SettingsRmlController::onSelectViewDistance(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    int tLevel = getIntArgument(pArguments);

    if (tLevel < 0)
    {
        return;
    }

    mDisplaySettings.setViewDistanceLevel(static_cast<size_t>(tLevel));
    mDisplaySettings.save();
    refreshViewDistanceOptions();
}

void SettingsRmlController::onToggleVsync(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mDisplaySettings.setVsyncEnabled(!mDisplaySettings.isVsyncEnabled());
    mDisplaySettings.applyVsync();
    mDisplaySettings.save();

    mIsVsyncEnabled = mDisplaySettings.isVsyncEnabled();
    mModelHandle.DirtyVariable("is_vsync_enabled");
}

void SettingsRmlController::onToggleCameraShake(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mDisplaySettings.setCameraShakeEnabled(!mDisplaySettings.isCameraShakeEnabled());
    mDisplaySettings.save();

    mIsCameraShakeEnabled = mDisplaySettings.isCameraShakeEnabled();
    mModelHandle.DirtyVariable("is_camera_shake_enabled");
}

void SettingsRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}
