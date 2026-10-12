#include "ReconnectRmlController.h"

#include <SDL3/SDL_log.h>

#include <string>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

ReconnectRmlController::ReconnectRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("reconnect");

    if (!tConstructor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "RmlUi data model 'reconnect' could not be created");
        return;
    }

    tConstructor.Bind("seconds_left", &mSecondsLeftText);
    tConstructor.BindEventCallback("logout", &ReconnectRmlController::onLogoutClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/reconnect.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

ReconnectRmlController::~ReconnectRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("reconnect");
    }
}

void ReconnectRmlController::setLogoutCallback(LogoutCallback pCallback)
{
    mLogoutCallback = std::move(pCallback);
}

void ReconnectRmlController::setSecondsLeft(int pSecondsLeft)
{
    if (pSecondsLeft == mSecondsLeft)
    {
        return;
    }

    mSecondsLeft = pSecondsLeft;
    mSecondsLeftText = std::to_string(pSecondsLeft);
    mModelHandle.DirtyVariable("seconds_left");
}

void ReconnectRmlController::onLogoutClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mLogoutCallback)
    {
        mLogoutCallback();
    }
}
