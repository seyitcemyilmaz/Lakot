#ifndef LAKOT_VITALSRMLCONTROLLER_H
#define LAKOT_VITALSRMLCONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>

#include <connection.pb.h>

namespace Rml
{
class ElementDocument;
}

namespace lakot
{

class RmlUiLayer;

class VitalsRmlController
{
public:
    using RespawnChoiceCallback = std::function<void(services::world::RespawnChoice pChoice)>;

    explicit VitalsRmlController(RmlUiLayer& pRmlUiLayer);
    ~VitalsRmlController();

    VitalsRmlController(const VitalsRmlController&) = delete;
    VitalsRmlController& operator=(const VitalsRmlController&) = delete;

    void setVitals(int32_t pHealth, int32_t pMaxHealth, uint32_t pLevel, uint64_t pExperience, uint64_t pExperienceForNextLevel);
    void setMana(int32_t pMana, int32_t pMaxMana);
    void setDeath(bool pIsDead, float pRespawnSeconds, float pAutoRespawnSeconds);
    void setRespawnChoiceCallback(RespawnChoiceCallback pCallback);
    void setSafeZone(bool pIsSafe);
    void showNotice(const std::string& pText);

    void update(double pDeltaTime);

private:
    static constexpr double kNoticeSeconds = 2.5;

    RmlUiLayer& mRmlUiLayer;
    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModel;

    Rml::String mLevel{"1"};
    Rml::String mHealth;
    Rml::String mHealthPercent{"100%"};
    Rml::String mManaPercent{"100%"};
    Rml::String mExperiencePercent{"0%"};
    Rml::String mRespawn;
    Rml::String mAutoRespawn;
    Rml::String mNotice;
    bool mIsDead{false};
    bool mIsSafe{false};
    bool mCanChoose{false};

    double mRespawnRemaining{0.0};
    double mAutoRespawnRemaining{0.0};

    RespawnChoiceCallback mRespawnChoiceCallback;

    void onReviveHereClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onReviveTownClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void requestRespawn(services::world::RespawnChoice pChoice);
    double mNoticeRemaining{0.0};
};

}

#endif
