#include "LanguageSettings.h"

#include <fstream>

#include <SDL3/SDL.h>

using namespace lakot;

void LanguageSettings::load()
{
    mLanguage = "en";

    std::ifstream tFile(getFilePath());
    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        if (tLine.rfind("language=", 0) != 0)
        {
            continue;
        }

        std::string tValue = tLine.substr(9);

        while (!tValue.empty() && (tValue.back() == '\r' || tValue.back() == ' '))
        {
            tValue.pop_back();
        }

        if (!tValue.empty())
        {
            mLanguage = tValue;
        }
    }
}

void LanguageSettings::save() const
{
    std::ofstream tFile(getFilePath(), std::ios::trunc);

    if (!tFile)
    {
        SDL_Log("Language setting could not be saved to %s", getFilePath().c_str());
        return;
    }

    tFile << "language=" << mLanguage << "\n";
}

const std::string& LanguageSettings::getLanguage() const
{
    return mLanguage;
}

void LanguageSettings::setLanguage(const std::string& pCode)
{
    mLanguage = pCode;
}

std::string LanguageSettings::getFilePath()
{
    std::string tPath;

    if (char* tPrefPath = SDL_GetPrefPath("Lakot", "LakotGame"))
    {
        tPath = tPrefPath;
        SDL_free(tPrefPath);
    }

    return tPath + "language.cfg";
}
