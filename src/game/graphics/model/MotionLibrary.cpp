#include "MotionLibrary.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <SDL3/SDL_log.h>

#include "AnimationRetargeter.h"
#include "SkinnedModel.h"

#include "../../asset/AssetManager.h"

using namespace lakot;

namespace
{
    bool readString(const rapidjson::Value& pObject, const char* pKey, std::string& pOut, std::string& pError)
    {
        if (!pObject.HasMember(pKey))
        {
            return true;
        }

        if (!pObject[pKey].IsString())
        {
            pError = std::string(pKey) + " must be a string";
            return false;
        }

        pOut = pObject[pKey].GetString();
        return true;
    }

    bool readNumber(const rapidjson::Value& pObject, const char* pKey, float& pOut, std::string& pError)
    {
        if (!pObject.HasMember(pKey))
        {
            return true;
        }

        if (!pObject[pKey].IsNumber())
        {
            pError = std::string(pKey) + " must be a number";
            return false;
        }

        float tValue = static_cast<float>(pObject[pKey].GetDouble());

        if (!std::isfinite(tValue))
        {
            pError = std::string(pKey) + " must be a finite number";
            return false;
        }

        pOut = tValue;
        return true;
    }

    bool readEventType(const std::string& pName, ClipEventType& pOut)
    {
        if (pName == "hit") { pOut = ClipEventType::eHit; return true; }
        if (pName == "effect") { pOut = ClipEventType::eEffect; return true; }
        if (pName == "sound") { pOut = ClipEventType::eSound; return true; }
        if (pName == "trail_on") { pOut = ClipEventType::eTrailOn; return true; }
        if (pName == "trail_off") { pOut = ClipEventType::eTrailOff; return true; }
        return false;
    }

    bool readEvents(const rapidjson::Value& pClip, std::vector<ClipEvent>& pOut, std::string& pError)
    {
        if (!pClip.HasMember("events"))
        {
            return true;
        }

        if (!pClip["events"].IsArray())
        {
            pError = "events must be an array";
            return false;
        }

        for (const auto& tEntry : pClip["events"].GetArray())
        {
            ClipEvent tEvent;
            std::string tType;

            if (!tEntry.IsObject() || !tEntry.HasMember("time") || !readNumber(tEntry, "time", tEvent.time, pError)
                || !readString(tEntry, "type", tType, pError) || !readString(tEntry, "name", tEvent.name, pError)
                || !readString(tEntry, "socket", tEvent.socket, pError))
            {
                pError = pError.empty() ? "an event needs a time" : pError;
                return false;
            }

            if (tEvent.time < 0.0f || !readEventType(tType, tEvent.type))
            {
                pError = "an event has a bad time or type '" + tType + "'";
                return false;
            }

            pOut.push_back(std::move(tEvent));
        }

        return true;
    }

    bool readStringMap(const rapidjson::Value& pDocument, const char* pKey, std::unordered_map<std::string, std::string>& pOut, std::string& pError)
    {
        if (!pDocument.HasMember(pKey))
        {
            return true;
        }

        if (!pDocument[pKey].IsObject())
        {
            pError = std::string(pKey) + " must be an object";
            return false;
        }

        for (const auto& tMember : pDocument[pKey].GetObject())
        {
            if (!tMember.value.IsString())
            {
                pError = std::string(pKey) + "." + tMember.name.GetString() + " must be a string";
                return false;
            }

            pOut[tMember.name.GetString()] = tMember.value.GetString();
        }

        return true;
    }

    bool readDefinitions(const rapidjson::Value& pDocument, std::unordered_map<std::string, ClipDefinition>& pOut, std::string& pError)
    {
        if (!pDocument.HasMember("clips"))
        {
            return true;
        }

        if (!pDocument["clips"].IsObject())
        {
            pError = "clips must be an object";
            return false;
        }

        for (const auto& tMember : pDocument["clips"].GetObject())
        {
            std::string tName = tMember.name.GetString();
            const rapidjson::Value& tClip = tMember.value;
            ClipDefinition tDefinition;
            tDefinition.source = tName;
            std::string tLayer;

            if (!tClip.IsObject() || !readString(tClip, "source", tDefinition.source, pError) || !readNumber(tClip, "start", tDefinition.start, pError)
                || !readNumber(tClip, "end", tDefinition.end, pError) || !readNumber(tClip, "rate", tDefinition.rate, pError)
                || !readString(tClip, "layer", tLayer, pError) || !readEvents(tClip, tDefinition.events, pError))
            {
                pError = "clip " + tName + ": " + (pError.empty() ? "must be an object" : pError);
                return false;
            }

            if (tClip.HasMember("loop"))
            {
                if (!tClip["loop"].IsBool())
                {
                    pError = "clip " + tName + ": loop must be a boolean";
                    return false;
                }

                tDefinition.isLoop = tClip["loop"].GetBool();
            }

            if (tDefinition.rate <= 0.0f || tDefinition.start < 0.0f || (!tLayer.empty() && tLayer != "full" && tLayer != "upper"))
            {
                pError = "clip " + tName + ": bad rate, start or layer";
                return false;
            }

            tDefinition.isUpper = tLayer == "upper";
            pOut.emplace(std::move(tName), std::move(tDefinition));
        }

        return true;
    }
}

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

    std::string tDefinitionError;

    if (!readDefinitions(tDocument, mDefinitions, tDefinitionError) || !readStringMap(tDocument, "roles", mRoles, tDefinitionError)
        || !readStringMap(tDocument, "skills", mSkillClips, tDefinitionError))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s ignored: %s", pManifestPath.c_str(), tDefinitionError.c_str());
        mDefinitions.clear();
        mRoles.clear();
        mSkillClips.clear();
    }

    return true;
}

const ClipDefinition* MotionLibrary::findClipDefinition(const std::string& pName) const
{
    auto tFound = mDefinitions.find(pName);
    return tFound != mDefinitions.end() ? &tFound->second : nullptr;
}

const std::unordered_map<std::string, std::string>& MotionLibrary::getRoles() const
{
    return mRoles;
}

const std::string* MotionLibrary::findSkillClip(const std::string& pSkillKey) const
{
    auto tFound = mSkillClips.find(pSkillKey);
    return tFound != mSkillClips.end() ? &tFound->second : nullptr;
}

bool MotionLibrary::hasClip(const std::string& pName) const
{
    const ClipDefinition* tDefinition = findClipDefinition(pName);
    return mSources.find(tDefinition ? tDefinition->source : pName) != mSources.end();
}

const AnimationClip* MotionLibrary::getClip(const SkinnedModel& pTarget, const std::string& pName)
{
    const ClipDefinition* tDefinition = findClipDefinition(pName);
    const std::string& tSourceName = tDefinition ? tDefinition->source : pName;
    auto tSource = mSources.find(tSourceName);

    if (tSource == mSources.end())
    {
        return nullptr;
    }

    auto& tCache = mRetargeted[&pTarget.getSkeleton()];
    auto tCached = tCache.find(tSourceName);

    if (tCached == tCache.end())
    {
        tCached = tCache.emplace(tSourceName, AnimationRetargeter::retarget(*tSource->second.clip,
                                                                     tSource->second.model->getSkeleton(),
                                                                     pTarget.getSkeleton())).first;
    }

    return tCached->second.get();
}
