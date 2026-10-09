#include "VitalsRmlController.h"

#include <algorithm>
#include <cmath>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>

#include "RmlUiLayer.h"

using namespace lakot;

namespace
{
    Rml::String toPercent(double pValue, double pMaximum)
    {
        double tRatio = pMaximum > 0.0 ? std::clamp(pValue / pMaximum, 0.0, 1.0) : 0.0;
        return std::to_string(static_cast<int>(std::round(tRatio * 100.0))) + "%";
    }
}

VitalsRmlController::VitalsRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("vitals");
    tConstructor.Bind("level", &mLevel);
    tConstructor.Bind("health", &mHealth);
    tConstructor.Bind("health_percent", &mHealthPercent);
    tConstructor.Bind("experience_percent", &mExperiencePercent);
    tConstructor.Bind("respawn", &mRespawn);
    tConstructor.Bind("notice", &mNotice);
    tConstructor.Bind("is_dead", &mIsDead);
    tConstructor.Bind("is_safe", &mIsSafe);
    mModel = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/vitals.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }
}

VitalsRmlController::~VitalsRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("vitals");
    }
}

void VitalsRmlController::setVitals(int32_t pHealth, int32_t pMaxHealth, uint32_t pLevel, uint64_t pExperience, uint64_t pExperienceForNextLevel)
{
    mLevel = std::to_string(pLevel);
    mHealth = std::to_string(std::max(0, pHealth)) + " / " + std::to_string(pMaxHealth);
    mHealthPercent = toPercent(pHealth, pMaxHealth);
    mExperiencePercent = toPercent(static_cast<double>(pExperience), static_cast<double>(pExperienceForNextLevel));

    mModel.DirtyVariable("level");
    mModel.DirtyVariable("health");
    mModel.DirtyVariable("health_percent");
    mModel.DirtyVariable("experience_percent");
}

void VitalsRmlController::setDeath(bool pIsDead, float pRespawnSeconds)
{
    mIsDead = pIsDead;
    mRespawnRemaining = pRespawnSeconds;
    mModel.DirtyVariable("is_dead");
}

void VitalsRmlController::setSafeZone(bool pIsSafe)
{
    if (mIsSafe != pIsSafe)
    {
        mIsSafe = pIsSafe;
        mModel.DirtyVariable("is_safe");
    }
}

void VitalsRmlController::showNotice(const std::string& pText)
{
    mNotice = pText;
    mNoticeRemaining = kNoticeSeconds;
    mModel.DirtyVariable("notice");
}

void VitalsRmlController::update(double pDeltaTime)
{
    if (mIsDead)
    {
        mRespawnRemaining = std::max(0.0, mRespawnRemaining - pDeltaTime);
        Rml::String tRespawn = std::to_string(static_cast<int>(std::ceil(mRespawnRemaining)));

        if (tRespawn != mRespawn)
        {
            mRespawn = tRespawn;
            mModel.DirtyVariable("respawn");
        }
    }

    if (mNoticeRemaining > 0.0)
    {
        mNoticeRemaining -= pDeltaTime;

        if (mNoticeRemaining <= 0.0)
        {
            mNotice.clear();
            mModel.DirtyVariable("notice");
        }
    }
}
