#ifndef LAKOT_WORLD_OVERLAY_H
#define LAKOT_WORLD_OVERLAY_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>

namespace Rml
{
class Element;
class ElementDocument;
}

namespace lakot
{

class RmlUiLayer;

class WorldOverlay
{
public:
    enum class NameToneType
    {
        ePlayer,
        eHostilePlayer,
        eNpc,
        eMonster,
        eGroundItem,
        eDead
    };

    enum class NumberToneType
    {
        eAgainstLocal,
        eByLocal,
        eOther
    };

    using MenuChatCallback = std::function<void(uint64_t pPlayerId, const std::string& pName)>;

    explicit WorldOverlay(RmlUiLayer& pRmlUiLayer);
    ~WorldOverlay();

    WorldOverlay(const WorldOverlay&) = delete;
    WorldOverlay& operator=(const WorldOverlay&) = delete;

    void setMenuChatCallback(MenuChatCallback pCallback);

    void beginFrame();
    void showNameplate(float pX, float pY, const std::string& pText, NameToneType pTone, int pHealthPercent, const std::string& pSpeech);
    void showDamageNumber(float pX, float pY, const std::string& pText, NumberToneType pTone, float pAlpha);
    void endFrame();

    void openMenu(float pX, float pY, uint64_t pPlayerId, const std::string& pName);
    void closeMenu();
    bool isMenuOpen() const;

private:
    struct Plate
    {
        Rml::Element* root{nullptr};
        Rml::Element* name{nullptr};
        Rml::Element* barFill{nullptr};
        Rml::Element* speech{nullptr};
        std::string text;
        std::string speechText;
        NameToneType tone{NameToneType::ePlayer};
        int health{-1};
        bool isVisible{true};
        bool hasToneApplied{false};
    };

    struct Number
    {
        Rml::Element* root{nullptr};
        std::string text;
        NumberToneType tone{NumberToneType::eOther};
        bool isVisible{true};
        bool hasToneApplied{false};
    };

    RmlUiLayer& mRmlUiLayer;
    Rml::ElementDocument* mDocument{nullptr};
    Rml::Element* mPlateParent{nullptr};
    Rml::Element* mNumberParent{nullptr};
    Rml::DataModelHandle mModel;

    std::vector<Plate> mPlates;
    std::vector<Number> mNumbers;
    size_t mPlatesUsed{0};
    size_t mNumbersUsed{0};

    bool mIsMenuOpen{false};
    Rml::String mMenuName;
    Rml::String mMenuLeft;
    Rml::String mMenuTop;
    uint64_t mMenuPlayerId{0};
    MenuChatCallback mMenuChatCallback;

    Plate& acquirePlate();
    Number& acquireNumber();
    void onMenuChatClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
};

}

#endif
