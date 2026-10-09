#ifndef LAKOT_VITALSRMLCONTROLLER_H
#define LAKOT_VITALSRMLCONTROLLER_H

#include <cstdint>
#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

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
    explicit VitalsRmlController(RmlUiLayer& pRmlUiLayer);
    ~VitalsRmlController();

    VitalsRmlController(const VitalsRmlController&) = delete;
    VitalsRmlController& operator=(const VitalsRmlController&) = delete;

    void setVitals(int32_t pHealth, int32_t pMaxHealth, uint32_t pLevel, uint64_t pExperience, uint64_t pExperienceForNextLevel);
    void setDeath(bool pIsDead, float pRespawnSeconds);
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
    Rml::String mExperiencePercent{"0%"};
    Rml::String mRespawn;
    Rml::String mNotice;
    bool mIsDead{false};
    bool mIsSafe{false};

    double mRespawnRemaining{0.0};
    double mNoticeRemaining{0.0};
};

}

#endif
