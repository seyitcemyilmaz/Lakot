#include "SkillBarRmlController.h"

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include "RmlUiLayer.h"

using namespace lakot;

namespace
{
    constexpr const char* kSlotElementIds[] = {
        "skill-slot-1", "skill-slot-2", "skill-slot-3", "skill-slot-4", "skill-slot-5",
        "skill-slot-f1", "skill-slot-f2", "skill-slot-f3", "skill-slot-f4", "skill-slot-f5"
    };

    constexpr int kLeftMouseButton = 0;
    constexpr int kRightMouseButton = 1;
}

SkillBarRmlController::SkillBarRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    mDocument = mRmlUiLayer.loadDocument("ui/skillbar.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);

        for (size_t tIndex = 0; tIndex < mSlotElements.size(); tIndex++)
        {
            mSlotElements[tIndex] = mDocument->GetElementById(kSlotElementIds[tIndex]);

            if (mSlotElements[tIndex])
            {
                mIconElements[tIndex] = mSlotElements[tIndex]->QuerySelector(".slot-icon");
                mCountElements[tIndex] = mSlotElements[tIndex]->QuerySelector(".slot-count");
                mCooldownElements[tIndex] = mSlotElements[tIndex]->QuerySelector(".slot-cooldown");

                mSlotElements[tIndex]->AddEventListener("mousedown", this);
                mSlotElements[tIndex]->AddEventListener("mouseup", this);
            }
        }
    }
}

SkillBarRmlController::~SkillBarRmlController()
{
    for (Rml::Element* tElement : mSlotElements)
    {
        if (tElement)
        {
            tElement->RemoveEventListener("mousedown", this);
            tElement->RemoveEventListener("mouseup", this);
        }
    }

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext && mDocument)
    {
        tContext->UnloadDocument(mDocument);
    }
}

void SkillBarRmlController::setSlotActivatedCallback(SlotActivatedCallback pCallback)
{
    mSlotActivatedCallback = std::move(pCallback);
}

void SkillBarRmlController::setSlotDroppedCallback(SlotDroppedCallback pCallback)
{
    mSlotDroppedCallback = std::move(pCallback);
}

void SkillBarRmlController::setSlot(SkillBarSlot pSlot, const std::string& pIconPath, uint32_t pCount)
{
    size_t tIndex = static_cast<size_t>(pSlot);

    if (tIndex >= kSlotCount || !mSlotElements[tIndex])
    {
        return;
    }

    mIsBound[tIndex] = true;

    mSlotElements[tIndex]->SetClass("empty", false);
    mSlotElements[tIndex]->SetClass("depleted", pCount == 0);

    if (mIconElements[tIndex])
    {
        mIconElements[tIndex]->SetAttribute("src", pIconPath);
    }

    if (mCountElements[tIndex])
    {
        mCountElements[tIndex]->SetInnerRML(pCount > 1 ? std::to_string(pCount) : std::string());
    }
}

void SkillBarRmlController::clearSlot(SkillBarSlot pSlot)
{
    size_t tIndex = static_cast<size_t>(pSlot);

    if (tIndex >= kSlotCount || !mSlotElements[tIndex])
    {
        return;
    }

    mIsBound[tIndex] = false;
    mCooldownRemainingSeconds[tIndex] = 0.0;

    mSlotElements[tIndex]->SetClass("empty", true);
    mSlotElements[tIndex]->SetClass("depleted", false);
    mSlotElements[tIndex]->SetClass("cooldown", false);

    if (mIconElements[tIndex])
    {
        mIconElements[tIndex]->RemoveAttribute("src");
    }

    if (mCountElements[tIndex])
    {
        mCountElements[tIndex]->SetInnerRML(std::string());
    }

    if (mCooldownElements[tIndex])
    {
        mCooldownElements[tIndex]->RemoveProperty("height");
    }
}

void SkillBarRmlController::startCooldown(SkillBarSlot pSlot, double pSeconds)
{
    size_t tIndex = static_cast<size_t>(pSlot);

    if (tIndex >= kSlotCount || !mSlotElements[tIndex] || !mIsBound[tIndex] || pSeconds <= 0.0)
    {
        return;
    }

    mCooldownRemainingSeconds[tIndex] = pSeconds;
    mCooldownTotalSeconds[tIndex] = pSeconds;

    mSlotElements[tIndex]->SetClass("cooldown", true);

    if (mCooldownElements[tIndex])
    {
        mCooldownElements[tIndex]->SetProperty("height", "100%");
    }
}

bool SkillBarRmlController::isCoolingDown(SkillBarSlot pSlot) const
{
    size_t tIndex = static_cast<size_t>(pSlot);

    return tIndex < kSlotCount && mCooldownRemainingSeconds[tIndex] > 0.0;
}

std::optional<SkillBarSlot> SkillBarRmlController::getSlotAt(float pX, float pY) const
{
    for (size_t tIndex = 0; tIndex < kSlotCount; tIndex++)
    {
        Rml::Element* tElement = mSlotElements[tIndex];

        if (!tElement)
        {
            continue;
        }

        float tLeft = tElement->GetAbsoluteLeft();
        float tTop = tElement->GetAbsoluteTop();

        if (pX >= tLeft && pY >= tTop && pX < tLeft + tElement->GetOffsetWidth() && pY < tTop + tElement->GetOffsetHeight())
        {
            return static_cast<SkillBarSlot>(tIndex);
        }
    }

    return std::nullopt;
}

void SkillBarRmlController::triggerFlash(SkillBarSlot pSlot)
{
    size_t tIndex = static_cast<size_t>(pSlot);

    if (tIndex >= mSlotElements.size() || !mSlotElements[tIndex])
    {
        return;
    }

    mSlotElements[tIndex]->SetClass("flashing", true);
    mFlashRemainingSeconds[tIndex] = kFlashDurationSeconds;
}

std::optional<SkillBarSlot> SkillBarRmlController::findSlot(const Rml::Element* pElement) const
{
    for (size_t tIndex = 0; tIndex < mSlotElements.size(); tIndex++)
    {
        if (mSlotElements[tIndex] == pElement)
        {
            return static_cast<SkillBarSlot>(tIndex);
        }
    }

    return std::nullopt;
}

void SkillBarRmlController::ProcessEvent(Rml::Event& pEvent)
{
    int tButton = pEvent.GetParameter<int>("button", -1);
    std::optional<SkillBarSlot> tSlot = findSlot(pEvent.GetCurrentElement());

    if (!tSlot)
    {
        return;
    }

    if (pEvent.GetId() == Rml::EventId::Mousedown)
    {
        if (tButton == kLeftMouseButton)
        {
            mPressedSlot = tSlot;
        }

        return;
    }

    if (tButton == kRightMouseButton && mSlotActivatedCallback)
    {
        mSlotActivatedCallback(*tSlot);
    }
}

void SkillBarRmlController::updatePressedSlot()
{
    if (!mPressedSlot)
    {
        return;
    }

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    bool tIsLeftHeld = (SDL_GetMouseState(&tMouseX, &tMouseY) & SDL_BUTTON_LMASK) != 0;

    if (tIsLeftHeld)
    {
        return;
    }

    SkillBarSlot tPressed = *mPressedSlot;
    mPressedSlot.reset();

    std::optional<SkillBarSlot> tReleasedOn = getSlotAt(tMouseX, tMouseY);

    if (tReleasedOn == tPressed)
    {
        if (mSlotActivatedCallback)
        {
            mSlotActivatedCallback(tPressed);
        }

        return;
    }

    if (mIsBound[static_cast<size_t>(tPressed)] && mSlotDroppedCallback)
    {
        mSlotDroppedCallback(tPressed, tReleasedOn);
    }
}

void SkillBarRmlController::updateCooldown(size_t pIndex, double pDeltaTime)
{
    if (mCooldownRemainingSeconds[pIndex] <= 0.0)
    {
        return;
    }

    mCooldownRemainingSeconds[pIndex] -= pDeltaTime;

    if (mCooldownRemainingSeconds[pIndex] <= 0.0)
    {
        mCooldownRemainingSeconds[pIndex] = 0.0;
        mSlotElements[pIndex]->SetClass("cooldown", false);

        if (mCooldownElements[pIndex])
        {
            mCooldownElements[pIndex]->RemoveProperty("height");
        }

        return;
    }

    if (mCooldownElements[pIndex])
    {
        double tFraction = mCooldownRemainingSeconds[pIndex] / mCooldownTotalSeconds[pIndex];
        mCooldownElements[pIndex]->SetProperty("height", std::to_string(static_cast<int>(tFraction * 100.0)) + "%");
    }
}

void SkillBarRmlController::update(double pDeltaTime)
{
    updatePressedSlot();

    for (size_t tIndex = 0; tIndex < mFlashRemainingSeconds.size(); tIndex++)
    {
        if (!mSlotElements[tIndex])
        {
            continue;
        }

        updateCooldown(tIndex, pDeltaTime);

        if (mFlashRemainingSeconds[tIndex] <= 0.0)
        {
            continue;
        }

        mFlashRemainingSeconds[tIndex] -= pDeltaTime;

        if (mFlashRemainingSeconds[tIndex] <= 0.0)
        {
            mSlotElements[tIndex]->SetClass("flashing", false);
        }
    }
}
