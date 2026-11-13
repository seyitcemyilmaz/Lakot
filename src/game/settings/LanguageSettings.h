#ifndef LAKOT_LANGUAGE_SETTINGS_H
#define LAKOT_LANGUAGE_SETTINGS_H

#include <string>

namespace lakot
{

class LanguageSettings
{
public:
    void load();
    void save() const;

    const std::string& getLanguage() const;
    void setLanguage(const std::string& pCode);

private:
    std::string mLanguage{"en"};

    static std::string getFilePath();
};

}

#endif
