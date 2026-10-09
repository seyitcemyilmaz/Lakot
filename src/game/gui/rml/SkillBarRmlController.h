#ifndef LAKOT_SKILLBARRMLCONTROLLER_H
#define LAKOT_SKILLBARRMLCONTROLLER_H

#include <array>

#include <RmlUi/Core/EventListener.h>

namespace Rml
{
class ElementDocument;
class Element;
class Event;
}

namespace lakot
{

class RmlUiLayer;

// One slot per skillbar.rml element id ("skill-slot-1".."skill-slot-f5"),
// in on-screen left-to-right order.
enum class SkillBarSlot
{
    eSlot1,
    eSlot2,
    eSlot3,
    eSlot4,
    eSlot5,
    eSlotF1,
    eSlotF2,
    eSlotF3,
    eSlotF4,
    eSlotF5,
    eCount
};

// Bottom-center HUD skill bar (1-5 / F1-F5 slots) - loads skillbar.rml and
// shows it. Item/skill binding itself is still a later pass (per the user's
// own "we'll do the rest in order") - the only behavior here so far is
// triggerFlash()'s "this slot was just activated" feedback, reachable two
// ways: WorldScene::handleEvent's keyboard case block (1-5/F1-F5 - which,
// same as the existing WASD handling right above it, never even runs while
// an RmlUi text input has focus, since RmlUiLayer::handleEvent already
// consumes the key event first; see its own comment for why - that's what
// keeps this from firing while typing in chat, with no extra check needed
// here), and this class's own ProcessEvent() for a right-click on the slot.
// Deliberately the SAME class/color/timer for both - the user was explicit
// that right-click and the matching keybind represent one and the same
// action and must look identical, not two different "kinds" of feedback.
//
// Wraps the loaded document in RAII (unloads it in the destructor) rather
// than leaving a bare Rml::ElementDocument* unmanaged, consistent with every
// other *RmlController in this directory - this session already found and
// fixed a real crash from an RmlUi resource outliving its owner (see
// SceneManager::shutdown()), so this isn't speculative caution.
//
// Implements Rml::EventListener directly (rather than adding a data model
// just to route one event) to also support right-clicking a slot - verified
// in the isolated rmlrepro harness that RmlUi's "click" event does NOT fire
// for the right mouse button, only "mousedown"/"mouseup" do (with a "button"
// parameter, 0=left/1=right/2=middle per RmlSDL::ConvertMouseButton), so
// slots listen for "mouseup" and check that parameter instead.
class SkillBarRmlController : public Rml::EventListener
{
public:
    explicit SkillBarRmlController(RmlUiLayer& pRmlUiLayer);
    ~SkillBarRmlController() override;

    SkillBarRmlController(const SkillBarRmlController&) = delete;
    SkillBarRmlController& operator=(const SkillBarRmlController&) = delete;

    // Applies the "flashing" CSS class to pSlot right away (Element::SetClass,
    // no data model involved) and schedules it to come back off shortly -
    // see update(). Calling this again on an already-flashing slot just
    // restarts the timer, so holding a key down briefly doesn't cut the
    // effect short. Also called from ProcessEvent() for a right-click on the
    // slot - same class, same effect, on purpose (see class comment).
    void triggerFlash(SkillBarSlot pSlot);

    // Call once per frame (WorldScene::update()) - counts down each
    // currently-flashing slot's remaining time and clears the class once it
    // reaches zero. .skillbar-slot's existing `transition` (already there
    // for :hover) makes both the on-set and the clear fade smoothly, so this
    // alone is the whole visual effect - no @keyframes animation needed.
    void update(double pDeltaTime);

private:
    static constexpr double kFlashDurationSeconds = 0.15;

    // Rml::EventListener - fires for "mouseup" on any of mSlotElements (see
    // AddEventListener calls in the constructor). Right-click (button == 1)
    // calls the same triggerFlash() a keyboard press does; other buttons are
    // ignored here since left-click has no bound action yet either.
    void ProcessEvent(Rml::Event& pEvent) override;

    RmlUiLayer& mRmlUiLayer;
    Rml::ElementDocument* mDocument{nullptr};

    std::array<Rml::Element*, static_cast<size_t>(SkillBarSlot::eCount)> mSlotElements{};
    std::array<double, static_cast<size_t>(SkillBarSlot::eCount)> mFlashRemainingSeconds{};
};

}

#endif
