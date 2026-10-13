#include "CharacterSelectScene.h"

#include "LoginScene.h"
#include "WorldScene.h"

#include "../Engine.h"
#include "../gui/PanelLoader.h"

using namespace lakot;

CharacterSelectScene::~CharacterSelectScene()
{

}

CharacterSelectScene::CharacterSelectScene(Engine& pEngine)
    : Scene(pEngine)
{

}

void CharacterSelectScene::enter()
{
    SDL_Log("CharacterSelectScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::Login);

    mController = std::make_unique<CharacterSelectRmlController>(mEngine.getRmlUiLayer(), mEngine.getMapCatalog(), mEngine.getClassCatalog());

    AuthController& tAuthController = mEngine.getNetworkManager().getAuthController();

    // Server -> UI.
    tAuthController.setCharacterListCallback(
    [this](const std::vector<AuthController::CharacterSummary>& pCharacters, uint32_t pMaxSlots, uint32_t pKingdom)
    {
        mController->setCharacters(pCharacters, pMaxSlots, pKingdom);
    });

    tAuthController.setCharacterCreateCallback(
    [this](bool pIsSuccess, const std::string& pMessage, const AuthController::CharacterSummary& pCharacter)
    {
        mController->onCreateResult(pIsSuccess, pMessage, pCharacter);
    });

    tAuthController.setCharacterDeleteCallback(
    [this](bool pIsSuccess, const std::string& pMessage, uint64_t pCharacterId)
    {
        mController->onDeleteResult(pIsSuccess, pMessage, pCharacterId);
    });

    tAuthController.setEnterWorldCallback(
    [this](bool pIsSuccess, const std::string& pMessage, const AuthController::EnterWorldResult& pResult)
    {
        if (!pIsSuccess)
        {
            mController->onEnterWorldFailed(pMessage);
            return;
        }

        // The server has already put this character into a zone by the time
        // this arrives, so the scene change is not optional - there is no
        // "stay here" branch once EnterWorld succeeds.
        WorldScene::InitialSpawnState tSpawnState{ pResult.mapId, pResult.x, pResult.y, pResult.z, pResult.yaw };

        auto tWorldScene = std::make_unique<WorldScene>(mEngine, tSpawnState, pResult.name);
        tWorldScene->setCharacterState(pResult.characterId, pResult.stats, pResult.items);

        mEngine.getSceneManager().setNextScene(std::move(tWorldScene));
    });

    // UI -> server.
    mController->setEnterWorldRequestCallback(
    [this](uint64_t pCharacterId)
    {
        mEngine.getNetworkManager().getAuthController().sendEnterWorld(pCharacterId);
    });

    mController->setCreateRequestCallback(
    [this](const std::string& pName, uint32_t pKingdom, uint32_t pClassId)
    {
        mEngine.getNetworkManager().getAuthController().sendCharacterCreate(pName, pKingdom, pClassId);
    });

    mController->setDeleteRequestCallback(
    [this](uint64_t pCharacterId, const std::string& pConfirmName)
    {
        mEngine.getNetworkManager().getAuthController().sendDeleteCharacter(pCharacterId, pConfirmName);
    });

    mController->setLogoutCallback(
    [this]()
    {
        mEngine.getNetworkManager().logout();
        mEngine.getSceneManager().setNextScene(std::make_unique<LoginScene>(mEngine));
    });

    // Nothing is on screen until this comes back - the list is the whole
    // point of the scene.
    tAuthController.requestCharacterList();
}

void CharacterSelectScene::exit()
{
    SDL_Log("CharacterSelectScene: exit");

    // Before destroying the controller: AuthController outlives this scene
    // (Engine owns it for the whole process) and the callbacks above capture
    // this scene's `this`, so leaving them registered would point them at
    // freed memory the moment any further auth response arrived.
    mEngine.getNetworkManager().getAuthController().clearCallbacks();

    mController.reset();
}

void CharacterSelectScene::update(double pDeltaTime)
{

}

void CharacterSelectScene::render()
{

}

bool CharacterSelectScene::handleEvent(SDL_Event* pEvent)
{
    return false;
}
