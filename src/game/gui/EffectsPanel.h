#ifndef LAKOT_EFFECTS_PANEL_H
#define LAKOT_EFFECTS_PANEL_H

#include <string>
#include <vector>

#include "Panel.h"

#include "../effect/EffectSystem.h"

namespace lakot
{

class WorldEffects;
class WorldEntities;

struct EffectsPanel : public Panel
{
    EffectsPanel(WorldEffects& pEffects, const WorldEntities& pEntities, std::vector<std::string> pReloadPaths);

    void render() override;

private:
    static constexpr int kStressEmitters = 200;
    static constexpr int kStressColumns = 15;
    static constexpr float kStressSpacing = 2.0f;
    static constexpr double kStressReportSeconds = 5.0;

    WorldEffects& mEffects;
    const WorldEntities& mEntities;
    std::vector<std::string> mReloadPaths;
    std::string mStatus;
    int mSelectedEffect{0};

    bool mIsStressing{false};
    double mStressStartTime{0.0};
    double mParticleSum{0.0};
    uint32_t mFrameCount{0};

    void renderStats();
    void startStress();
    void stopStress();
    void updateStress(const EffectStats& pStats);
    void reload();
};

}

#endif
