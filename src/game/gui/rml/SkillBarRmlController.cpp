#include "SkillBarRmlController.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include "RmlUiLayer.h"

using namespace lakot;

namespace
{
    // Parallel to the SkillBarSlot enum order - see skillbar.rml.
    constexpr const char* kSlotElementIds[] = {
        "skill-slot-1", "skill-slot-2", "skill-slot-3", "skill-slot-4", "skill-slot-5",
        "skill-slot-f1", "skill-slot-f2", "skill-slot-f3", "skill-slot-f4", "skill-slot-f5"
    };
}

SkillBarRmlController::SkillBarRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    mDocument = mRmlUiLayer.loadDocument("ui/skillbar.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);

        for (size_t i = 0; i < mSlotElements.size(); i++)
        {
            mSlotElements[i] = mDocument->GetElementById(kSlotElementIds[i]);

            if (mSlotElements[i])
            {
                mSlotElements[i]->AddEventListener("mouseup", this);
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
            tElement->RemoveEventListener("mouseup", this);
        }
    }

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext && mDocument)
    {
        tContext->UnloadDocument(mDocument);
    }
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

void SkillBarRmlController::ProcessEvent(Rml::Event& pEvent)
{
    // button: 0=left, 1=right, 2=middle (RmlSDL::ConvertMouseButton) - only
    // right-click is bound to anything so far, per the user's request.
    if (pEvent.GetParameter<int>("button", -1) != 1)
    {
        return;
    }

    Rml::Element* tCurrentElement = pEvent.GetCurrentElement();

    for (size_t i = 0; i < mSlotElements.size(); i++)
    {
        if (mSlotElements[i] == tCurrentElement)
        {
            triggerFlash(static_cast<SkillBarSlot>(i));
            return;
        }
    }
}

void SkillBarRmlController::update(double pDeltaTime)
{
    for (size_t i = 0; i < mFlashRemainingSeconds.size(); i++)
    {
        if (mFlashRemainingSeconds[i] <= 0.0)
        {
            continue;
        }

        mFlashRemainingSeconds[i] -= pDeltaTime;

        if (mFlashRemainingSeconds[i] <= 0.0 && mSlotElements[i])
        {
            mSlotElements[i]->SetClass("flashing", false);
        }
    }
}
