#ifndef LAKOT_RMLSYSTEMINTERFACE_H
#define LAKOT_RMLSYSTEMINTERFACE_H

#include "backend/RmlUi_Platform_SDL.h"

namespace lakot
{

class Localization;

class RmlSystemInterface final : public SystemInterface_SDL
{
public:
    explicit RmlSystemInterface(const Localization& pLocalization);

    int TranslateString(Rml::String& pTranslated, const Rml::String& pInput) override;

private:
    const Localization& mLocalization;
};

}

#endif
