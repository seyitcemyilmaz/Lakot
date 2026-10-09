#ifndef LAKOT_CHARACTER_SHEET_RML_CONTROLLER_H
#define LAKOT_CHARACTER_SHEET_RML_CONTROLLER_H

#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "../../network/GameTypes.h"

namespace lakot
{

class RmlUiLayer;

// The character sheet, toggled with C. Split out of the inventory panel,
// which was carrying both the bag and the stats and had to shrink its grid to
// unreadable cells to fit them.
//
// Purely a display of what the server last sent. Every number here, derived
// ones included, comes from the server - nothing is recomputed client-side,
// so the sheet can never disagree with what the server thinks the character
// is.
class CharacterSheetRmlController
{
public:
    CharacterSheetRmlController(RmlUiLayer& pRmlUiLayer, const std::string& pCharacterName);
    ~CharacterSheetRmlController();

    CharacterSheetRmlController(const CharacterSheetRmlController&) = delete;
    CharacterSheetRmlController& operator=(const CharacterSheetRmlController&) = delete;

    void setStats(const CharacterStats& pStats);

    void toggle();
    void close();
    bool isOpen() const;

private:
    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    Rml::String mName;
    Rml::String mLevel;

    Rml::String mHealth, mMana, mExperience;

    // Bar widths as percentage strings, applied through data-style - the bar
    // is drawn by CSS and only its length is data.
    Rml::String mHealthPercent{"0%"}, mManaPercent{"0%"}, mExperiencePercent{"0%"};

    Rml::String mStrength, mDexterity, mIntelligence, mVitality;
    Rml::String mAttack, mDefense, mGold;

    bool mIsOpen{false};

    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void dirtyAll();

    // Clamped to 0..100 so a value that somehow exceeds its maximum cannot
    // draw a bar past the end of its track.
    static Rml::String toPercent(int64_t pValue, int64_t pMaximum);
};

}

#endif
