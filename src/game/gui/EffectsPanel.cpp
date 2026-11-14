#include "EffectsPanel.h"

#include <algorithm>

#include <SDL3/SDL_log.h>

#include "../scene/world/WorldEffects.h"
#include "../scene/world/WorldEntities.h"

using namespace lakot;

namespace
{
    constexpr float kUpdateBudgetMs = 0.5f;
    constexpr float kBuildBudgetMs = 0.5f;
    constexpr float kGpuBudgetMs = 1.0f;
    constexpr float kTotalBudgetMs = 2.0f;
}

EffectsPanel::EffectsPanel(WorldEffects& pEffects, const WorldEntities& pEntities, std::vector<std::string> pReloadPaths)
    : Panel("Effects (F9)", ImGuiWindowFlags_AlwaysAutoResize)
    , mEffects(pEffects)
    , mEntities(pEntities)
    , mReloadPaths(std::move(pReloadPaths))
{
    isOpen = false;
}

void EffectsPanel::render()
{
    renderStats();

    ImGui::SeparatorText("Play");

    const std::vector<EffectDefinition>& tEffects = mEffects.getSystem().getCatalog().getEffects();
    const WorldEntity* tLocal = mEntities.getLocal();

    if (!tEffects.empty())
    {
        mSelectedEffect = std::min(mSelectedEffect, static_cast<int>(tEffects.size()) - 1);

        if (ImGui::BeginCombo("Effect", tEffects[mSelectedEffect].name.c_str()))
        {
            for (int tIndex = 0; tIndex < static_cast<int>(tEffects.size()); tIndex++)
            {
                if (ImGui::Selectable(tEffects[tIndex].name.c_str(), tIndex == mSelectedEffect))
                {
                    mSelectedEffect = tIndex;
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Play at player") && tLocal)
        {
            mEffects.getSystem().spawn(tEffects[mSelectedEffect].name, WorldEntities::getPosition(*tLocal));
        }
    }

    ImGui::SeparatorText("Stress");

    if (ImGui::Button("Stress 200") && tLocal)
    {
        startStress();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear"))
    {
        stopStress();
        mEffects.getSystem().clear();
    }

    ImGui::Separator();

    if (ImGui::Button("Reload effects.json"))
    {
        reload();
    }

    if (!mStatus.empty())
    {
        ImGui::TextUnformatted(mStatus.c_str());
    }
}

void EffectsPanel::renderStats()
{
    EffectStats tStats = mEffects.getSystem().getStats();
    ParticleRenderStats tRender = mEffects.getRenderer().getStats();

    ImGui::Text("Emitters: %u  Particles: %u", tStats.emitterCount, tStats.particleCount);
    ImGui::Text("Update: %.3f ms  Build: %.3f ms", tStats.updateMs, tRender.buildMs);

    if (tRender.gpuMs >= 0.0f)
    {
        ImGui::Text("GPU: %.3f ms", tRender.gpuMs);
    }
    else
    {
        ImGui::TextUnformatted("GPU: n/a");
    }

    ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);

    if (mIsStressing)
    {
        updateStress(tStats);
    }
}

void EffectsPanel::startStress()
{
    stopStress();

    const WorldEntity* tLocal = mEntities.getLocal();

    if (!tLocal)
    {
        return;
    }

    glm::vec3 tCenter = WorldEntities::getPosition(*tLocal);
    int tRows = (kStressEmitters + kStressColumns - 1) / kStressColumns;

    for (int tIndex = 0; tIndex < kStressEmitters; tIndex++)
    {
        float tX = (static_cast<float>(tIndex % kStressColumns) - static_cast<float>(kStressColumns - 1) * 0.5f) * kStressSpacing;
        float tZ = (static_cast<float>(tIndex / kStressColumns) - static_cast<float>(tRows - 1) * 0.5f) * kStressSpacing;
        mEffects.getSystem().spawn("stress_loop", tCenter + glm::vec3(tX, 0.0f, tZ));
    }

    mIsStressing = true;
    mStressStartTime = ImGui::GetTime();
    mParticleSum = 0.0;
    mFrameCount = 0;
}

void EffectsPanel::stopStress()
{
    mIsStressing = false;
}

void EffectsPanel::updateStress(const EffectStats& pStats)
{
    mParticleSum += pStats.particleCount;
    mFrameCount++;

    if (ImGui::GetTime() - mStressStartTime < kStressReportSeconds)
    {
        return;
    }

    ParticleRenderStats tRender = mEffects.getRenderer().getStats();
    float tGpu = tRender.gpuMs >= 0.0f ? tRender.gpuMs : 0.0f;
    float tTotal = pStats.updateMs + tRender.buildMs + tGpu;
    bool tIsPassing = pStats.updateMs <= kUpdateBudgetMs && tRender.buildMs <= kBuildBudgetMs && tGpu <= kGpuBudgetMs && tTotal <= kTotalBudgetMs;

    SDL_Log("EffectSystem stress: emitters=%u particles=%.0f update=%.3f build=%.3f gpu=%.3f budget=%s",
            pStats.emitterCount, mParticleSum / static_cast<double>(std::max(mFrameCount, 1u)), pStats.updateMs, tRender.buildMs,
            tRender.gpuMs, tIsPassing ? "PASS" : "FAIL");

    mIsStressing = false;
}

void EffectsPanel::reload()
{
    mStatus = "No effects.json found";

    for (const std::string& tPath : mReloadPaths)
    {
        std::string tError;

        if (mEffects.reload(tPath, tError))
        {
            mStatus = "Reloaded " + tPath;
            return;
        }

        mStatus = tError;
    }
}
