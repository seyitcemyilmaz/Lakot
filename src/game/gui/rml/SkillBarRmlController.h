#ifndef LAKOT_SKILLBARRMLCONTROLLER_H
#define LAKOT_SKILLBARRMLCONTROLLER_H

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

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

class SkillBarRmlController : public Rml::EventListener
{
public:
    using SlotActivatedCallback = std::function<void(SkillBarSlot pSlot)>;
    using SlotDroppedCallback = std::function<void(SkillBarSlot pFrom, std::optional<SkillBarSlot> pTo)>;

    static constexpr size_t kSlotCount = static_cast<size_t>(SkillBarSlot::eCount);

    explicit SkillBarRmlController(RmlUiLayer& pRmlUiLayer);
    ~SkillBarRmlController() override;

    SkillBarRmlController(const SkillBarRmlController&) = delete;
    SkillBarRmlController& operator=(const SkillBarRmlController&) = delete;

    void setSlotActivatedCallback(SlotActivatedCallback pCallback);
    void setSlotDroppedCallback(SlotDroppedCallback pCallback);

    void setSlot(SkillBarSlot pSlot, const std::string& pIconPath, uint32_t pCount);
    void clearSlot(SkillBarSlot pSlot);

    void startCooldown(SkillBarSlot pSlot, double pSeconds);
    bool isCoolingDown(SkillBarSlot pSlot) const;

    std::optional<SkillBarSlot> getSlotAt(float pX, float pY) const;

    void triggerFlash(SkillBarSlot pSlot);

    void update(double pDeltaTime);

private:
    static constexpr double kFlashDurationSeconds = 0.15;

    void ProcessEvent(Rml::Event& pEvent) override;

    std::optional<SkillBarSlot> findSlot(const Rml::Element* pElement) const;
    void updatePressedSlot();
    void updateCooldown(size_t pIndex, double pDeltaTime);

    RmlUiLayer& mRmlUiLayer;
    Rml::ElementDocument* mDocument{nullptr};

    std::array<Rml::Element*, kSlotCount> mSlotElements{};
    std::array<Rml::Element*, kSlotCount> mIconElements{};
    std::array<Rml::Element*, kSlotCount> mCountElements{};
    std::array<Rml::Element*, kSlotCount> mCooldownElements{};
    std::array<double, kSlotCount> mFlashRemainingSeconds{};
    std::array<double, kSlotCount> mCooldownRemainingSeconds{};
    std::array<double, kSlotCount> mCooldownTotalSeconds{};
    std::array<bool, kSlotCount> mIsBound{};

    std::optional<SkillBarSlot> mPressedSlot;

    SlotActivatedCallback mSlotActivatedCallback;
    SlotDroppedCallback mSlotDroppedCallback;
};

}

#endif
