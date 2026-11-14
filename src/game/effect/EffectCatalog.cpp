#include "EffectCatalog.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    constexpr size_t kMaxNameLength = 64;
    constexpr uint32_t kMaxEmittersPerEffect = 8;
    constexpr float kMaxParticlesPerEmitter = 256.0f;

    bool isValidName(const std::string& pName, size_t pMaxLength)
    {
        if (pName.empty() || pName.size() > pMaxLength)
        {
            return false;
        }

        return std::all_of(pName.begin(), pName.end(), [](char pChar)
        {
            return (pChar >= 'a' && pChar <= 'z') || (pChar >= '0' && pChar <= '9') || pChar == '_';
        });
    }

    bool readNumber(const rapidjson::Value& pValue, float& pResult)
    {
        if (!pValue.IsNumber())
        {
            return false;
        }

        pResult = pValue.GetFloat();
        return std::isfinite(pResult);
    }

    const rapidjson::Value* findMember(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember == pObject.MemberEnd() ? nullptr : &tMember->value;
    }

    std::string readString(const rapidjson::Value& pObject, const char* pName)
    {
        const rapidjson::Value* tValue = findMember(pObject, pName);
        return tValue && tValue->IsString() ? tValue->GetString() : "";
    }

    bool readFloat(const rapidjson::Value& pObject, const char* pName, float pMin, float pMax, float& pResult, std::string& pError)
    {
        const rapidjson::Value* tValue = findMember(pObject, pName);

        if (!tValue || !readNumber(*tValue, pResult) || pResult < pMin || pResult > pMax)
        {
            pError = std::string("field ") + pName + " is missing or out of range";
            return false;
        }

        return true;
    }

    bool readRange(const rapidjson::Value& pObject, const char* pName, float pMin, float pMax, FloatRange& pResult, std::string& pError)
    {
        const rapidjson::Value* tValue = findMember(pObject, pName);

        if (!tValue || !tValue->IsArray() || tValue->Size() != 2
            || !readNumber((*tValue)[0], pResult.min) || !readNumber((*tValue)[1], pResult.max)
            || pResult.min < pMin || pResult.max > pMax || pResult.min > pResult.max)
        {
            pError = std::string("range ") + pName + " is missing or invalid";
            return false;
        }

        return true;
    }

    bool readVector(const rapidjson::Value& pObject, const char* pName, glm::vec3& pResult, std::string& pError)
    {
        const rapidjson::Value* tValue = findMember(pObject, pName);

        if (!tValue || !tValue->IsArray() || tValue->Size() != 3
            || !readNumber((*tValue)[0], pResult.x) || !readNumber((*tValue)[1], pResult.y) || !readNumber((*tValue)[2], pResult.z))
        {
            pError = std::string("vector ") + pName + " is missing or invalid";
            return false;
        }

        return true;
    }

    bool readKeyTime(const rapidjson::Value& pKey, float pPreviousTime, float& pTime)
    {
        return pKey.IsArray() && pKey.Size() == 2 && readNumber(pKey[0], pTime) && pTime >= 0.0f && pTime <= 1.0f && pTime > pPreviousTime;
    }

    bool readSizeCurve(const rapidjson::Value& pObject, Curve<float>& pResult, std::string& pError)
    {
        const rapidjson::Value* tValue = findMember(pObject, "size");

        if (!tValue || !tValue->IsArray() || tValue->Size() < 1 || tValue->Size() > 4)
        {
            pError = "size curve needs 1..4 keys";
            return false;
        }

        pResult.count = tValue->Size();
        float tPreviousTime = -1.0f;

        for (uint32_t tIndex = 0; tIndex < pResult.count; tIndex++)
        {
            const rapidjson::Value& tKey = (*tValue)[tIndex];
            CurveKey<float>& tOut = pResult.keys[tIndex];

            if (!readKeyTime(tKey, tPreviousTime, tOut.time) || !readNumber(tKey[1], tOut.value) || tOut.value < 0.0f || tOut.value > 100.0f)
            {
                pError = "size curve key is invalid";
                return false;
            }

            tPreviousTime = tOut.time;
        }

        return true;
    }

    bool readColorCurve(const rapidjson::Value& pObject, Curve<glm::vec4>& pResult, std::string& pError)
    {
        const rapidjson::Value* tValue = findMember(pObject, "color");

        if (!tValue || !tValue->IsArray() || tValue->Size() < 1 || tValue->Size() > 4)
        {
            pError = "color curve needs 1..4 keys";
            return false;
        }

        pResult.count = tValue->Size();
        float tPreviousTime = -1.0f;

        for (uint32_t tIndex = 0; tIndex < pResult.count; tIndex++)
        {
            const rapidjson::Value& tKey = (*tValue)[tIndex];
            CurveKey<glm::vec4>& tOut = pResult.keys[tIndex];

            bool tIsValid = readKeyTime(tKey, tPreviousTime, tOut.time) && tKey[1].IsArray() && tKey[1].Size() == 4;

            for (rapidjson::SizeType tComponent = 0; tIsValid && tComponent < 4; tComponent++)
            {
                tIsValid = readNumber(tKey[1][tComponent], tOut.value[tComponent]) && tOut.value[tComponent] >= 0.0f && tOut.value[tComponent] <= 4.0f;
            }

            if (!tIsValid)
            {
                pError = "color curve key is invalid";
                return false;
            }

            tPreviousTime = tOut.time;
        }

        return true;
    }

    bool readShape(const rapidjson::Value& pObject, EmitterDefinition& pResult, std::string& pError)
    {
        const rapidjson::Value* tShape = findMember(pObject, "shape");

        if (!tShape || !tShape->IsObject())
        {
            pError = "shape is missing";
            return false;
        }

        std::string tTypeName = readString(*tShape, "type");

        if (tTypeName == "point")
        {
            pResult.shape = EmitterShapeType::ePoint;
        }
        else if (tTypeName == "sphere")
        {
            pResult.shape = EmitterShapeType::eSphere;
        }
        else if (tTypeName == "ring")
        {
            pResult.shape = EmitterShapeType::eRing;
        }
        else
        {
            pError = "unknown shape type " + tTypeName;
            return false;
        }

        return readFloat(*tShape, "radius", 0.0f, 10.0f, pResult.shapeRadius, pError);
    }

    bool readEnums(const rapidjson::Value& pObject, const std::vector<std::string>& pTextures, EmitterDefinition& pResult, std::string& pError)
    {
        std::string tTextureName = readString(pObject, "texture");
        auto tTextureIterator = std::find(pTextures.begin(), pTextures.end(), tTextureName);

        if (tTextureIterator == pTextures.end())
        {
            pError = "unknown texture " + tTextureName;
            return false;
        }

        pResult.textureIndex = static_cast<uint32_t>(tTextureIterator - pTextures.begin());

        std::string tBlendName = readString(pObject, "blend");

        if (tBlendName == "additive")
        {
            pResult.blend = BlendType::eAdditive;
        }
        else if (tBlendName == "alpha")
        {
            pResult.blend = BlendType::eAlpha;
        }
        else
        {
            pError = "unknown blend " + tBlendName;
            return false;
        }

        std::string tOrientationName = readString(pObject, "orientation");

        if (tOrientationName == "billboard")
        {
            pResult.orientation = ParticleOrientationType::eBillboard;
        }
        else if (tOrientationName == "velocity")
        {
            pResult.orientation = ParticleOrientationType::eVelocity;
        }
        else if (tOrientationName == "ground")
        {
            pResult.orientation = ParticleOrientationType::eGround;
        }
        else
        {
            pError = "unknown orientation " + tOrientationName;
            return false;
        }

        return true;
    }

    bool readEmitter(const rapidjson::Value& pObject, const std::vector<std::string>& pTextures, EmitterDefinition& pResult, std::string& pError)
    {
        if (!pObject.IsObject() || !readEnums(pObject, pTextures, pResult, pError))
        {
            if (pError.empty())
            {
                pError = "emitter is not an object";
            }

            return false;
        }

        float tBurst = 0.0f;
        const rapidjson::Value* tLoop = findMember(pObject, "loop");

        bool tIsValid = readFloat(pObject, "burst", 0.0f, 256.0f, tBurst, pError)
                     && readFloat(pObject, "rate", 0.0f, 500.0f, pResult.rate, pError)
                     && readFloat(pObject, "duration", 0.0f, 30.0f, pResult.duration, pError)
                     && readRange(pObject, "lifetime", 0.01f, 10.0f, pResult.lifetime, pError)
                     && readRange(pObject, "speed", 0.0f, 1000.0f, pResult.speed, pError)
                     && readVector(pObject, "direction", pResult.direction, pError)
                     && readFloat(pObject, "spread", 0.0f, 180.0f, pResult.spreadDegrees, pError)
                     && readFloat(pObject, "gravity", -1000.0f, 1000.0f, pResult.gravity, pError)
                     && readFloat(pObject, "drag", 0.0f, 1000.0f, pResult.drag, pError)
                     && readFloat(pObject, "stretch", 0.0f, 1000.0f, pResult.stretch, pError)
                     && readShape(pObject, pResult, pError)
                     && readVector(pObject, "offset", pResult.offset, pError)
                     && readSizeCurve(pObject, pResult.size, pError)
                     && readColorCurve(pObject, pResult.color, pError)
                     && readRange(pObject, "rotation", -3600.0f, 3600.0f, pResult.rotationDegrees, pError)
                     && readRange(pObject, "spin", -3600.0f, 3600.0f, pResult.spinDegrees, pError);

        if (!tIsValid)
        {
            return false;
        }

        if (!tLoop || !tLoop->IsBool())
        {
            pError = "field loop is missing";
            return false;
        }

        pResult.burst = static_cast<uint32_t>(tBurst);
        pResult.isLoop = tLoop->GetBool();

        if (pResult.burst == 0 && pResult.rate <= 0.0f)
        {
            pError = "emitter needs burst or rate";
            return false;
        }

        if (static_cast<float>(pResult.burst) + std::ceil(pResult.rate * pResult.lifetime.max) > kMaxParticlesPerEmitter)
        {
            pError = "emitter can hold more than 256 particles";
            return false;
        }

        return true;
    }

    bool readTextures(const rapidjson::Document& pDocument, std::vector<std::string>& pResult, std::string& pError)
    {
        auto tTextures = pDocument.FindMember("textures");

        if (tTextures == pDocument.MemberEnd() || !tTextures->value.IsArray() || tTextures->value.Empty())
        {
            pError = "textures list is missing";
            return false;
        }

        std::set<std::string> tSeen;

        for (const rapidjson::Value& tEntry : tTextures->value.GetArray())
        {
            std::string tName = tEntry.IsString() ? tEntry.GetString() : "";

            if (!isValidName(tName, kMaxNameLength) || !tSeen.insert(tName).second)
            {
                pError = "texture name " + tName + " is invalid or duplicated";
                return false;
            }

            pResult.push_back(tName);
        }

        return true;
    }

    bool readEffect(const rapidjson::Value& pEntry, const std::vector<std::string>& pTextures, EffectDefinition& pResult, std::string& pError)
    {
        pResult.name = pEntry.IsObject() ? readString(pEntry, "name") : "";

        if (!isValidName(pResult.name, kMaxNameLength))
        {
            pError = "effect name " + pResult.name + " is invalid";
            return false;
        }

        const rapidjson::Value* tEmitters = findMember(pEntry, "emitters");

        if (!tEmitters || !tEmitters->IsArray() || tEmitters->Empty() || tEmitters->Size() > kMaxEmittersPerEffect)
        {
            pError = "effect " + pResult.name + " needs 1..8 emitters";
            return false;
        }

        for (const rapidjson::Value& tEmitterValue : tEmitters->GetArray())
        {
            EmitterDefinition tEmitter;
            std::string tEmitterError;

            if (!readEmitter(tEmitterValue, pTextures, tEmitter, tEmitterError))
            {
                pError = "effect " + pResult.name + ": " + tEmitterError;
                return false;
            }

            pResult.emitters.push_back(tEmitter);
        }

        return true;
    }
}

bool EffectCatalog::load(const std::string& pPath, std::string& pError)
{
    std::ifstream tFile(pPath, std::ios::binary);

    if (!tFile)
    {
        pError = "cannot read " + pPath;
        return false;
    }

    std::ostringstream tBuffer;
    tBuffer << tFile.rdbuf();
    std::string tJson = tBuffer.str();

    rapidjson::Document tDocument;
    tDocument.Parse(tJson.c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject())
    {
        pError = pPath + " is not valid JSON";
        return false;
    }

    std::vector<std::string> tTextures;
    std::vector<EffectDefinition> tEffects;
    std::map<std::string, size_t, std::less<>> tIndexByName;
    std::string tError;

    auto tEffectList = tDocument.FindMember("effects");

    if (!readTextures(tDocument, tTextures, tError))
    {
        pError = pPath + ": " + tError;
        return false;
    }

    if (tEffectList == tDocument.MemberEnd() || !tEffectList->value.IsArray())
    {
        pError = pPath + ": effects list is missing";
        return false;
    }

    for (const rapidjson::Value& tEntry : tEffectList->value.GetArray())
    {
        EffectDefinition tEffect;

        if (!readEffect(tEntry, tTextures, tEffect, tError))
        {
            pError = pPath + ": " + tError;
            return false;
        }

        if (tIndexByName.count(tEffect.name))
        {
            pError = pPath + ": effect " + tEffect.name + " is duplicated";
            return false;
        }

        tIndexByName[tEffect.name] = tEffects.size();
        tEffects.push_back(std::move(tEffect));
    }

    mTextureNames = std::move(tTextures);
    mEffects = std::move(tEffects);
    mIndexByName = std::move(tIndexByName);
    return true;
}

const EffectDefinition* EffectCatalog::find(std::string_view pName) const
{
    auto tIterator = mIndexByName.find(pName);
    return tIterator == mIndexByName.end() ? nullptr : &mEffects[tIterator->second];
}

const std::vector<EffectDefinition>& EffectCatalog::getEffects() const
{
    return mEffects;
}

const std::vector<std::string>& EffectCatalog::getTextureNames() const
{
    return mTextureNames;
}
