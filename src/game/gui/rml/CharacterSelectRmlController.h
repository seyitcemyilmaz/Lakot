#ifndef LAKOT_CHARACTER_SELECT_RML_CONTROLLER_H
#define LAKOT_CHARACTER_SELECT_RML_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "MapCatalog.h"

#include "../../network/controller/AuthController.h"

namespace lakot
{

class RmlUiLayer;

// The screen between logging in and existing in the world: pick one of the
// account's characters, or create one first.
//
// It owns no networking of its own - the scene wires it to AuthController and
// feeds results back in. That keeps this class about presentation and lets
// the scene remain the single place that knows when to change scenes.
class CharacterSelectRmlController
{
public:
    // Raised when the player picks a character and the server has confirmed
    // it - the scene turns this into the switch to WorldScene.
    using EnterWorldRequestCallback = std::function<void(uint64_t pCharacterId)>;
    using CreateRequestCallback = std::function<void(const std::string& pName, uint32_t pKingdom)>;
    using LogoutCallback = std::function<void()>;

    CharacterSelectRmlController(RmlUiLayer& pRmlUiLayer, const MapCatalog& pMapCatalog);
    ~CharacterSelectRmlController();

    CharacterSelectRmlController(const CharacterSelectRmlController&) = delete;
    CharacterSelectRmlController& operator=(const CharacterSelectRmlController&) = delete;

    void setEnterWorldRequestCallback(EnterWorldRequestCallback pCallback);
    void setCreateRequestCallback(CreateRequestCallback pCallback);
    void setLogoutCallback(LogoutCallback pCallback);

    // Replaces the whole list - called on the initial listing and again after
    // a successful creation.
    // pKingdom: the account's kingdom, 0 while it still has to be chosen.
    void setCharacters(const std::vector<AuthController::CharacterSummary>& pCharacters, uint32_t pMaxSlots, uint32_t pKingdom);

    void onCreateResult(bool pIsSuccess, const std::string& pMessage, const AuthController::CharacterSummary& pCharacter);
    void onEnterWorldFailed(const std::string& pMessage);

    void setStatus(const std::string& pMessage, bool pIsError);
    void setBusy(bool pIsBusy);

private:
    // One row of the list. index (not the character id) is what the click
    // handler passes back: RmlUi's Variant has no 64-bit integer type, so a
    // character id cannot round-trip through a data event argument - the
    // position in mCharacters can, and this class maps it back.
    struct CharacterRow
    {
        int index = 0;
        Rml::String name;
        Rml::String location;
        bool isSelected = false;
    };

    // One choosable kingdom on the create form.
    struct KingdomRow
    {
        int id = 0;
        Rml::String name;
        Rml::String description;
        Rml::String color;
        bool isSelected = false;
    };

    RmlUiLayer& mRmlUiLayer;
    const MapCatalog& mMapCatalog;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    // The authoritative list, parallel to mRows - mRows is what the UI binds
    // to, this is what the ids are looked up from.
    std::vector<AuthController::CharacterSummary> mCharacters;
    std::vector<CharacterRow> mRows;
    std::vector<KingdomRow> mKingdomRows;

    // The account's kingdom (0 = not chosen yet) and, until then, the one
    // picked on the create form.
    uint32_t mKingdom{0};
    uint32_t mChosenKingdom{0};

    Rml::String mKingdomLabel;
    bool mHasKingdom{false};

    Rml::String mSlotsLabel;
    Rml::String mStatusMessage;
    Rml::String mNewName;

    bool mHasCharacters{false};
    bool mHasSelection{false};
    bool mHasFreeSlot{false};
    bool mIsCreating{false};
    bool mIsBusy{false};
    bool mIsError{false};

    int mSelectedIndex{-1};
    uint32_t mMaxSlots{0};

    EnterWorldRequestCallback mEnterWorldRequestCallback;
    CreateRequestCallback mCreateRequestCallback;
    LogoutCallback mLogoutCallback;

    void onSelectClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onEnterWorldClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onBeginCreateClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onConfirmCreateClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCancelCreateClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onLogoutClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onNameKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onChooseKingdomClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void doConfirmCreate();
    void selectIndex(int pIndex);
    void rebuildRows();
    void rebuildKingdomRows();
    void setKingdom(uint32_t pKingdom);
    void dirtyAll();

    std::string getMapName(uint32_t pMapId) const;
};

}

#endif
