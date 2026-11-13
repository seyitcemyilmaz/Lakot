#include "RmlSystemInterface.h"

#include "../../localization/Localization.h"

using namespace lakot;

RmlSystemInterface::RmlSystemInterface(const Localization& pLocalization)
    : mLocalization(pLocalization)
{

}

int RmlSystemInterface::TranslateString(Rml::String& pTranslated, const Rml::String& pInput)
{
    mLocalization.translateMarkup(pTranslated, pInput);
    return pTranslated != pInput ? 1 : 0;
}
