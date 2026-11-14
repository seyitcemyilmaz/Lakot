#include "RmlDocument.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include "RmlUiLayer.h"

using namespace lakot;

RmlDocument::~RmlDocument()
{
    reset();
}

void RmlDocument::load(RmlUiLayer& pRmlUiLayer, const std::string& pRelativePath)
{
    reset();
    mContext = pRmlUiLayer.getContext();
    mDocument = pRmlUiLayer.loadDocument(pRelativePath);
}

void RmlDocument::loadFromTemplate(RmlUiLayer& pRmlUiLayer, const std::string& pRelativePath, const std::string& pModelPlaceholder, const std::string& pModelName)
{
    reset();
    mContext = pRmlUiLayer.getContext();
    mDocument = pRmlUiLayer.loadDocumentFromTemplate(pRelativePath, pModelPlaceholder, pModelName);
}

void RmlDocument::reset()
{
    if (mContext && mDocument)
    {
        mContext->UnloadDocument(mDocument);
    }

    mDocument = nullptr;
}

Rml::ElementDocument* RmlDocument::get() const
{
    return mDocument;
}

Rml::ElementDocument* RmlDocument::operator->() const
{
    return mDocument;
}

RmlDocument::operator bool() const
{
    return mDocument != nullptr;
}
