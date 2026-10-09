#include "MotionLibrary.h"

#include <fstream>
#include <sstream>

#include <rapidjson/document.h>

#include "AnimationRetargeter.h"
#include "SkinnedModel.h"

#include "../../asset/AssetManager.h"

using namespace lakot;

MotionLibrary::MotionLibrary(AssetManager& pAssets)
    : mAssets(pAssets)
{

}

bool MotionLibrary::load(const std::string& pManifestPath, std::string& pError)
{
    std::ifstream tFile(mAssets.resolve(pManifestPath), std::ios::binary);

    if (!tFile)
    {
        pError = "cannot read " + pManifestPath;
        return false;
    }

    std::ostringstream tBuffer;
    tBuffer << tFile.rdbuf();
    std::string tJson = tBuffer.str();

    rapidjson::Document tDocument;
    tDocument.Parse(tJson.c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("sources") || !tDocument["sources"].IsArray())
    {
        pError = pManifestPath + " needs a sources array";
        return false;
    }

    for (const auto& tEntry : tDocument["sources"].GetArray())
    {
        if (!tEntry.IsString())
        {
            continue;
        }

        std::shared_ptr<SkinnedModel> tModel = mAssets.getMotionSource(tEntry.GetString());

        if (!tModel)
        {
            pError = std::string("cannot load motion source ") + tEntry.GetString();
            return false;
        }

        for (const AnimationClip& tClip : tModel->getClips())
        {
            mSources.emplace(tClip.getName(), Source{ tModel, &tClip });
        }
    }

    return true;
}

bool MotionLibrary::hasClip(const std::string& pName) const
{
    return mSources.find(pName) != mSources.end();
}

const AnimationClip* MotionLibrary::getClip(const SkinnedModel& pTarget, const std::string& pName)
{
    auto tSource = mSources.find(pName);

    if (tSource == mSources.end())
    {
        return nullptr;
    }

    auto& tCache = mRetargeted[&pTarget.getSkeleton()];
    auto tCached = tCache.find(pName);

    if (tCached == tCache.end())
    {
        tCached = tCache.emplace(pName, AnimationRetargeter::retarget(*tSource->second.clip,
                                                                     tSource->second.model->getSkeleton(),
                                                                     pTarget.getSkeleton())).first;
    }

    return tCached->second.get();
}
