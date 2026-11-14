#ifndef LAKOT_RMLDOCUMENT_H
#define LAKOT_RMLDOCUMENT_H

#include <string>

namespace Rml
{
class Context;
class ElementDocument;
}

namespace lakot
{

class RmlUiLayer;

class RmlDocument
{
public:
    RmlDocument() = default;
    ~RmlDocument();

    RmlDocument(const RmlDocument&) = delete;
    RmlDocument& operator=(const RmlDocument&) = delete;

    void load(RmlUiLayer& pRmlUiLayer, const std::string& pRelativePath);
    void loadFromTemplate(RmlUiLayer& pRmlUiLayer, const std::string& pRelativePath, const std::string& pModelPlaceholder, const std::string& pModelName);
    void reset();

    Rml::ElementDocument* get() const;
    Rml::ElementDocument* operator->() const;
    explicit operator bool() const;

private:
    Rml::Context* mContext{nullptr};
    Rml::ElementDocument* mDocument{nullptr};
};

}

#endif
