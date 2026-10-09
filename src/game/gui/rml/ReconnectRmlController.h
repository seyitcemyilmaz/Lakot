#ifndef LAKOT_RECONNECT_RML_CONTROLLER_H
#define LAKOT_RECONNECT_RML_CONTROLLER_H

#include <functional>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class RmlUiLayer;

// Full-window overlay shown while a dropped connection is being resumed.
// Visible for its whole lifetime - Engine creates it when reconnecting starts
// and destroys it when that ends either way.
class ReconnectRmlController
{
public:
    using LogoutCallback = std::function<void()>;

    explicit ReconnectRmlController(RmlUiLayer& pRmlUiLayer);
    ~ReconnectRmlController();

    ReconnectRmlController(const ReconnectRmlController&) = delete;
    ReconnectRmlController& operator=(const ReconnectRmlController&) = delete;

    void setLogoutCallback(LogoutCallback pCallback);

    void setSecondsLeft(int pSecondsLeft);

private:
    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    int mSecondsLeft{-1};
    Rml::String mSecondsLeftText;

    LogoutCallback mLogoutCallback;

    void onLogoutClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
};

}

#endif
