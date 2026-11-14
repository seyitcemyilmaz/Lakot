#include "CharacterSelectRmlController.h"

#include <algorithm>
#include <cctype>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>
#include <RmlUi/Core/Input.h>

#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

namespace
{
    std::string trimmed(const std::string& pText)
    {
        size_t tBegin = pText.find_first_not_of(" \t");

        if (tBegin == std::string::npos)
        {
            return {};
        }

        size_t tEnd = pText.find_last_not_of(" \t");

        return pText.substr(tBegin, tEnd - tBegin + 1);
    }

    std::string toLower(std::string pText)
    {
        std::transform(pText.begin(), pText.end(), pText.begin(), [](unsigned char pChar) { return static_cast<char>(std::tolower(pChar)); });
        return pText;
    }
}

CharacterSelectRmlController::CharacterSelectRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const MapCatalog& pMapCatalog, const ClassCatalog& pClassCatalog)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mMapCatalog(pMapCatalog)
    , mClassCatalog(pClassCatalog)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("character_select");

    // Same guard and reason as ChatRmlController::ChatLine: RmlUi's struct
    // type registry is shared per Rml::Context, not per model, so registering
    // the same C++ type twice logs a warning in Release and trips an assert
    // dialog in Debug. This screen is created and destroyed on every trip
    // back to the login screen, so it would hit that on the second visit.
    static bool sCharacterRowTypeRegistered = false;

    if (!sCharacterRowTypeRegistered)
    {
        Rml::StructHandle<CharacterRow> tRowHandle = tConstructor.RegisterStruct<CharacterRow>();
        tRowHandle.RegisterMember("index", &CharacterRow::index);
        tRowHandle.RegisterMember("name", &CharacterRow::name);
        tRowHandle.RegisterMember("location", &CharacterRow::location);
        tRowHandle.RegisterMember("class_name", &CharacterRow::className);
        tRowHandle.RegisterMember("meta_text", &CharacterRow::metaText);
        tRowHandle.RegisterMember("level", &CharacterRow::level);
        tRowHandle.RegisterMember("is_selected", &CharacterRow::isSelected);
        tConstructor.RegisterArray<std::vector<CharacterRow>>();

        Rml::StructHandle<KingdomRow> tKingdomHandle = tConstructor.RegisterStruct<KingdomRow>();
        tKingdomHandle.RegisterMember("id", &KingdomRow::id);
        tKingdomHandle.RegisterMember("name", &KingdomRow::name);
        tKingdomHandle.RegisterMember("description", &KingdomRow::description);
        tKingdomHandle.RegisterMember("color", &KingdomRow::color);
        tKingdomHandle.RegisterMember("is_selected", &KingdomRow::isSelected);
        tConstructor.RegisterArray<std::vector<KingdomRow>>();

        Rml::StructHandle<ClassRow> tClassHandle = tConstructor.RegisterStruct<ClassRow>();
        tClassHandle.RegisterMember("id", &ClassRow::id);
        tClassHandle.RegisterMember("name", &ClassRow::name);
        tClassHandle.RegisterMember("description", &ClassRow::description);
        tClassHandle.RegisterMember("is_selected", &ClassRow::isSelected);
        tConstructor.RegisterArray<std::vector<ClassRow>>();

        sCharacterRowTypeRegistered = true;
    }

    tConstructor.Bind("characters", &mRows);
    tConstructor.Bind("kingdoms", &mKingdomRows);
    tConstructor.Bind("classes", &mClassRows);
    tConstructor.Bind("has_class", &mHasClass);
    tConstructor.Bind("kingdom_label", &mKingdomLabel);
    tConstructor.Bind("has_kingdom", &mHasKingdom);
    tConstructor.Bind("slots_label", &mSlotsLabel);
    tConstructor.Bind("status_message", &mStatusMessage);
    tConstructor.Bind("new_name", &mNewName);
    tConstructor.Bind("has_characters", &mHasCharacters);
    tConstructor.Bind("has_selection", &mHasSelection);
    tConstructor.Bind("has_free_slot", &mHasFreeSlot);
    tConstructor.Bind("is_creating", &mIsCreating);
    tConstructor.Bind("is_deleting", &mIsDeleting);
    tConstructor.Bind("delete_target_name", &mDeleteTargetName);
    tConstructor.Bind("delete_target_meta", &mDeleteTargetMeta);
    tConstructor.Bind("delete_name_input", &mDeleteNameInput);
    tConstructor.Bind("delete_name_matches", &mDeleteNameMatches);
    tConstructor.Bind("is_busy", &mIsBusy);
    tConstructor.Bind("is_error", &mIsError);

    tConstructor.BindEventCallback("select", &CharacterSelectRmlController::onSelectClicked, this);
    tConstructor.BindEventCallback("enter_world", &CharacterSelectRmlController::onEnterWorldClicked, this);
    tConstructor.BindEventCallback("begin_create", &CharacterSelectRmlController::onBeginCreateClicked, this);
    tConstructor.BindEventCallback("confirm_create", &CharacterSelectRmlController::onConfirmCreateClicked, this);
    tConstructor.BindEventCallback("cancel_create", &CharacterSelectRmlController::onCancelCreateClicked, this);
    tConstructor.BindEventCallback("begin_delete", &CharacterSelectRmlController::onBeginDeleteClicked, this);
    tConstructor.BindEventCallback("cancel_delete", &CharacterSelectRmlController::onCancelDeleteClicked, this);
    tConstructor.BindEventCallback("confirm_delete", &CharacterSelectRmlController::onConfirmDeleteClicked, this);
    tConstructor.BindEventCallback("delete_name_changed", &CharacterSelectRmlController::onDeleteNameChanged, this);
    tConstructor.BindEventCallback("delete_name_keydown", &CharacterSelectRmlController::onDeleteNameKeyDown, this);
    tConstructor.BindEventCallback("logout", &CharacterSelectRmlController::onLogoutClicked, this);
    tConstructor.BindEventCallback("name_keydown", &CharacterSelectRmlController::onNameKeyDown, this);
    tConstructor.BindEventCallback("choose_kingdom", &CharacterSelectRmlController::onChooseKingdomClicked, this);
    tConstructor.BindEventCallback("choose_class", &CharacterSelectRmlController::onChooseClassClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    rebuildKingdomRows();
    rebuildClassRows();

    mDocument.load(mRmlUiLayer, "ui/characterselect.rml");

    if (mDocument)
    {
        mDocument->Show();
    }
}

CharacterSelectRmlController::~CharacterSelectRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("character_select");
    }
}

void CharacterSelectRmlController::setEnterWorldRequestCallback(EnterWorldRequestCallback pCallback)
{
    mEnterWorldRequestCallback = pCallback;
}

void CharacterSelectRmlController::setCreateRequestCallback(CreateRequestCallback pCallback)
{
    mCreateRequestCallback = pCallback;
}

void CharacterSelectRmlController::setDeleteRequestCallback(DeleteRequestCallback pCallback)
{
    mDeleteRequestCallback = pCallback;
}

void CharacterSelectRmlController::setLogoutCallback(LogoutCallback pCallback)
{
    mLogoutCallback = pCallback;
}

std::string CharacterSelectRmlController::getMapName(uint32_t pMapId) const
{
    const MapData* tMap = mMapCatalog.getMap(pMapId);
    return tMap ? mLocalization.content("map." + std::to_string(pMapId) + ".name", tMap->getName()) : mLocalization.text("characterselect.unknown_map");
}

std::string CharacterSelectRmlController::getClassName(uint32_t pClassId) const
{
    const ClassDefinition* tClass = mClassCatalog.find(pClassId);
    return tClass ? mLocalization.content("class." + tClass->key + ".name", tClass->name) : mLocalization.text("characterselect.classless");
}

void CharacterSelectRmlController::setKingdom(uint32_t pKingdom)
{
    mKingdom = pKingdom;

    const KingdomInfo* tKingdom = mMapCatalog.getKingdom(pKingdom);

    mHasKingdom = tKingdom != nullptr;
    mKingdomLabel = tKingdom ? mLocalization.format("characterselect.kingdom_of", { { "name", mLocalization.content("kingdom." + std::to_string(tKingdom->id) + ".name", tKingdom->name) } }) : "";
}

void CharacterSelectRmlController::rebuildKingdomRows()
{
    mKingdomRows.clear();

    for (const KingdomInfo& tKingdom : mMapCatalog.getKingdoms())
    {
        KingdomRow tRow;
        tRow.id = static_cast<int>(tKingdom.id);
        tRow.name = mLocalization.content("kingdom." + std::to_string(tKingdom.id) + ".name", tKingdom.name);
        tRow.description = mLocalization.content("kingdom." + std::to_string(tKingdom.id) + ".desc", tKingdom.description);
        tRow.color = tKingdom.color;
        tRow.isSelected = (tKingdom.id == mChosenKingdom);

        mKingdomRows.push_back(std::move(tRow));
    }
}

void CharacterSelectRmlController::rebuildClassRows()
{
    mClassRows.clear();

    for (const ClassDefinition& tClass : mClassCatalog.getAll())
    {
        std::string tMain;

        for (const std::string& tStat : tClass.mainStats)
        {
            std::string tLabel = getStatLabel(tStat);
            tMain += (tMain.empty() ? "" : " + ") + tLabel;
        }

        ClassRow tRow;
        tRow.id = static_cast<int>(tClass.id);
        tRow.name = mLocalization.content("class." + tClass.key + ".name", tClass.name);
        tRow.description = mLocalization.format("characterselect.class_stats",
            { { "main", tMain }, { "str", std::to_string(tClass.startStrength) }, { "dex", std::to_string(tClass.startDexterity) },
              { "int", std::to_string(tClass.startIntelligence) }, { "vit", std::to_string(tClass.startVitality) } });
        tRow.isSelected = (tClass.id == mChosenClass);

        mClassRows.push_back(std::move(tRow));
    }

    mHasClass = mClassCatalog.find(mChosenClass) != nullptr;
}

void CharacterSelectRmlController::onChooseClassClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (mIsBusy || pArguments.empty())
    {
        return;
    }

    uint32_t tClass = static_cast<uint32_t>(pArguments[0].Get<int>(0));

    if (!mClassCatalog.find(tClass))
    {
        return;
    }

    mChosenClass = tClass;

    rebuildClassRows();
    dirtyAll();
}

void CharacterSelectRmlController::onChooseKingdomClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (mIsBusy || pArguments.empty())
    {
        return;
    }

    uint32_t tKingdom = static_cast<uint32_t>(pArguments[0].Get<int>(0));

    if (!MapCatalog::isValidKingdom(tKingdom))
    {
        return;
    }

    mChosenKingdom = tKingdom;

    rebuildKingdomRows();
    dirtyAll();
}

std::string CharacterSelectRmlController::getStatLabel(const std::string& pStat) const
{
    if (pStat == "str")
    {
        return mLocalization.text("characterselect.stat.str");
    }

    if (pStat == "dex")
    {
        return mLocalization.text("characterselect.stat.dex");
    }

    if (pStat == "int")
    {
        return mLocalization.text("characterselect.stat.int");
    }

    if (pStat == "vit")
    {
        return mLocalization.text("characterselect.stat.vit");
    }

    std::string tUpper = pStat;
    std::transform(tUpper.begin(), tUpper.end(), tUpper.begin(), [](unsigned char pChar) { return static_cast<char>(std::toupper(pChar)); });

    return tUpper;
}

void CharacterSelectRmlController::setCharacters(const std::vector<AuthController::CharacterSummary>& pCharacters, uint32_t pMaxSlots, uint32_t pKingdom)
{
    setKingdom(pKingdom);

    mCharacters = pCharacters;
    mMaxSlots = pMaxSlots;

    mHasCharacters = !mCharacters.empty();
    mHasFreeSlot = mCharacters.size() < pMaxSlots;

    mSlotsLabel = mLocalization.format("characterselect.slots_used", { { "used", std::to_string(mCharacters.size()) }, { "max", std::to_string(pMaxSlots) } });

    // Preselecting the only character makes the common case (one character,
    // played every session) a single click instead of two.
    mSelectedIndex = mCharacters.empty() ? -1 : 0;

    rebuildRows();
    dirtyAll();
}

void CharacterSelectRmlController::rebuildRows()
{
    mRows.clear();
    mRows.reserve(mCharacters.size());

    for (size_t tIndex = 0; tIndex < mCharacters.size(); ++tIndex)
    {
        CharacterRow tRow;
        tRow.index = static_cast<int>(tIndex);
        tRow.name = mCharacters[tIndex].name;
        tRow.location = getMapName(mCharacters[tIndex].mapId);
        tRow.className = getClassName(mCharacters[tIndex].classId);
        tRow.level = static_cast<int>(mCharacters[tIndex].level);
        tRow.metaText = mLocalization.format("characterselect.row_meta", { { "class", tRow.className }, { "level", std::to_string(tRow.level) }, { "location", tRow.location } });
        tRow.isSelected = (static_cast<int>(tIndex) == mSelectedIndex);

        mRows.push_back(std::move(tRow));
    }

    mHasSelection = (mSelectedIndex >= 0 && mSelectedIndex < static_cast<int>(mCharacters.size()));
}

void CharacterSelectRmlController::selectIndex(int pIndex)
{
    if (pIndex < 0 || pIndex >= static_cast<int>(mCharacters.size()))
    {
        return;
    }

    mSelectedIndex = pIndex;

    rebuildRows();
    dirtyAll();
}

void CharacterSelectRmlController::onSelectClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (mIsBusy || pArguments.empty())
    {
        return;
    }

    selectIndex(pArguments[0].Get<int>(-1));
}

void CharacterSelectRmlController::onEnterWorldClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mIsBusy || !mHasSelection)
    {
        return;
    }

    setBusy(true);
    setStatus(mLocalization.text("characterselect.status.entering"), false);

    if (mEnterWorldRequestCallback)
    {
        mEnterWorldRequestCallback(mCharacters[mSelectedIndex].characterId);
    }
}

void CharacterSelectRmlController::onBeginCreateClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mIsBusy || !mHasFreeSlot)
    {
        return;
    }

    mIsCreating = true;
    mNewName.clear();
    mChosenClass = 0;

    rebuildClassRows();

    setStatus("", false);
}

void CharacterSelectRmlController::onConfirmCreateClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doConfirmCreate();
}

void CharacterSelectRmlController::onNameKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        doConfirmCreate();
    }
    else
    {
        return;
    }

    // Same reasoning as WorldChatRmlController::onInputKeyDown - RmlUi's own
    // text-input default action re-syncs the model from its internal buffer
    // right after this runs, undoing the clear in doConfirmCreate(). Plain
    // StopPropagation() is not enough; it only blocks ancestors, not other
    // listeners on this same element.
    pEvent.StopImmediatePropagation();
}

void CharacterSelectRmlController::doConfirmCreate()
{
    if (mIsBusy)
    {
        return;
    }

    std::string tName(mNewName);

    if (!mHasKingdom && !MapCatalog::isValidKingdom(mChosenKingdom))
    {
        setStatus(mLocalization.text("characterselect.status.need_kingdom"), true);
        return;
    }

    if (!mClassCatalog.find(mChosenClass))
    {
        setStatus(mLocalization.text("characterselect.status.need_class"), true);
        return;
    }

    // Matched to the server's own rule (RequestLimits::kMinUsernameLength /
    // kMaxUsernameLength) so the common mistake is caught without a round
    // trip - the server still enforces it, this is not the check that counts.
    if (tName.size() < 3 || tName.size() > 50)
    {
        setStatus(mLocalization.text("characterselect.status.name_length"), true);
        return;
    }

    setBusy(true);
    setStatus(mLocalization.text("characterselect.status.creating"), false);

    if (mCreateRequestCallback)
    {
        mCreateRequestCallback(tName, mHasKingdom ? mKingdom : mChosenKingdom, mChosenClass);
    }
}

void CharacterSelectRmlController::onCancelCreateClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mIsBusy)
    {
        return;
    }

    mIsCreating = false;
    mNewName.clear();

    setStatus("", false);
}

void CharacterSelectRmlController::onBeginDeleteClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mIsBusy || !mHasSelection)
    {
        return;
    }

    const AuthController::CharacterSummary& tCharacter = mCharacters[mSelectedIndex];

    mDeleteTargetId = tCharacter.characterId;
    mDeleteTargetName = tCharacter.name;
    mDeleteTargetMeta = mLocalization.format("characterselect.delete_meta", { { "class", getClassName(tCharacter.classId) }, { "level", std::to_string(tCharacter.level) }, { "location", getMapName(tCharacter.mapId) } });
    mDeleteNameInput.clear();
    mDeleteNameMatches = false;
    mIsDeleting = true;

    setStatus("", false);
}

void CharacterSelectRmlController::onCancelDeleteClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doCancelDelete();
}

void CharacterSelectRmlController::onConfirmDeleteClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doConfirmDelete();
}

void CharacterSelectRmlController::onDeleteNameChanged(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    updateDeleteMatch();
}

void CharacterSelectRmlController::onDeleteNameKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        doConfirmDelete();
    }
    else if (tKeyIdentifier == Rml::Input::KI_ESCAPE)
    {
        doCancelDelete();
    }
    else
    {
        return;
    }

    pEvent.StopImmediatePropagation();
}

void CharacterSelectRmlController::doCancelDelete()
{
    if (mIsBusy)
    {
        return;
    }

    mDeleteTargetId = 0;
    mIsDeleting = false;
    mDeleteNameInput.clear();
    mDeleteNameMatches = false;

    setStatus("", false);
}

void CharacterSelectRmlController::doConfirmDelete()
{
    updateDeleteMatch();

    if (mIsBusy || !mIsDeleting || !mDeleteNameMatches || mDeleteTargetId == 0)
    {
        return;
    }

    std::string tConfirmName = trimmed(mDeleteNameInput);

    setBusy(true);
    setStatus(mLocalization.text("characterselect.status.deleting"), false);

    if (mDeleteRequestCallback)
    {
        mDeleteRequestCallback(mDeleteTargetId, tConfirmName);
    }
}

void CharacterSelectRmlController::updateDeleteMatch()
{
    std::string tTyped = trimmed(mDeleteNameInput);

    mDeleteNameMatches = !tTyped.empty() && toLower(tTyped) == toLower(mDeleteTargetName);

    mModelHandle.DirtyVariable("delete_name_matches");
}

void CharacterSelectRmlController::onDeleteResult(bool pIsSuccess, const std::string& pMessage, uint64_t pCharacterId)
{
    setBusy(false);

    if (!pIsSuccess)
    {
        setStatus(pMessage, true);
        return;
    }

    auto tIterator = std::find_if(mCharacters.begin(), mCharacters.end(),
        [pCharacterId](const AuthController::CharacterSummary& pCharacter) { return pCharacter.characterId == pCharacterId; });

    if (tIterator != mCharacters.end())
    {
        mCharacters.erase(tIterator);
    }

    mHasCharacters = !mCharacters.empty();
    mHasFreeSlot = mCharacters.size() < mMaxSlots;
    mSlotsLabel = mLocalization.format("characterselect.slots_used", { { "used", std::to_string(mCharacters.size()) }, { "max", std::to_string(mMaxSlots) } });

    mSelectedIndex = mCharacters.empty() ? -1 : 0;
    mDeleteTargetId = 0;
    mIsDeleting = false;
    mDeleteNameInput.clear();
    mDeleteNameMatches = false;

    rebuildRows();
    setStatus(mLocalization.text("characterselect.status.deleted"), false);
}

void CharacterSelectRmlController::onLogoutClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mLogoutCallback)
    {
        mLogoutCallback();
    }
}

void CharacterSelectRmlController::onCreateResult(bool pIsSuccess, const std::string& pMessage, const AuthController::CharacterSummary& pCharacter)
{
    setBusy(false);

    if (!pIsSuccess)
    {
        setStatus(pMessage, true);
        return;
    }

    // The first character just fixed the account's kingdom.
    if (!mHasKingdom)
    {
        setKingdom(mChosenKingdom);
    }

    // Appended locally rather than re-requesting the whole list: the response
    // already carries the new character, and the server is the one that
    // decided the id and starting map.
    mCharacters.push_back(pCharacter);

    mHasCharacters = true;
    mHasFreeSlot = mCharacters.size() < mMaxSlots;
    mSlotsLabel = mLocalization.format("characterselect.slots_used", { { "used", std::to_string(mCharacters.size()) }, { "max", std::to_string(mMaxSlots) } });

    mSelectedIndex = static_cast<int>(mCharacters.size()) - 1;
    mIsCreating = false;
    mNewName.clear();

    rebuildRows();
    setStatus(mLocalization.text("characterselect.status.created"), false);
}

void CharacterSelectRmlController::onEnterWorldFailed(const std::string& pMessage)
{
    setBusy(false);
    setStatus(pMessage, true);
}

void CharacterSelectRmlController::setStatus(const std::string& pMessage, bool pIsError)
{
    mStatusMessage = pMessage;
    mIsError = pIsError;

    dirtyAll();
}

void CharacterSelectRmlController::setBusy(bool pIsBusy)
{
    mIsBusy = pIsBusy;

    dirtyAll();
}

void CharacterSelectRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();
}
