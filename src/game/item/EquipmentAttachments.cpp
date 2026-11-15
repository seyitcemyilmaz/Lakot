#include "EquipmentAttachments.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "../graphics/model/SkinnedModel.h"

using namespace lakot;

namespace
{
    glm::vec3 getVector(const rapidjson::Value& pObject, const char* pName, const glm::vec3& pFallback)
    {
        auto tMember = pObject.FindMember(pName);

        if (tMember == pObject.MemberEnd() || !tMember->value.IsArray() || tMember->value.Size() != 3)
        {
            return pFallback;
        }

        return glm::vec3(tMember->value[0].GetFloat(), tMember->value[1].GetFloat(), tMember->value[2].GetFloat());
    }

    float getFloat(const rapidjson::Value& pObject, const char* pName, float pFallback)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsNumber() ? tMember->value.GetFloat() : pFallback;
    }

    AttachmentRule readRuleValue(const rapidjson::Value& pRule, const AttachmentRule& pBase)
    {
        AttachmentRule tResult;
        tResult.joint = pRule.HasMember("joint") && pRule["joint"].IsString() ? pRule["joint"].GetString() : pBase.joint;
        tResult.length = getFloat(pRule, "length", pBase.length);
        tResult.grip = getFloat(pRule, "grip", pBase.grip);
        tResult.widthScale = getFloat(pRule, "widthScale", pBase.widthScale);
        tResult.rotationDegrees = getVector(pRule, "rotation", pBase.rotationDegrees);
        tResult.offset = getVector(pRule, "offset", pBase.offset);
        return tResult;
    }

    std::optional<AttachmentRule> readRule(const rapidjson::Value& pParent, const char* pName)
    {
        auto tMember = pParent.FindMember(pName);

        if (tMember == pParent.MemberEnd() || !tMember->value.IsObject())
        {
            return std::nullopt;
        }

        return readRuleValue(tMember->value, AttachmentRule());
    }

    void readOverrides(const rapidjson::Value& pDocument, const char* pSlotName, const std::optional<AttachmentRule>& pGlobal,
                       std::map<std::string, AttachmentRule>& pTarget)
    {
        pTarget.clear();

        auto tOverrides = pDocument.FindMember("overrides");

        if (tOverrides == pDocument.MemberEnd() || !tOverrides->value.IsObject())
        {
            return;
        }

        auto tSlot = tOverrides->value.FindMember(pSlotName);

        if (tSlot == tOverrides->value.MemberEnd() || !tSlot->value.IsObject())
        {
            return;
        }

        for (auto tEntry = tSlot->value.MemberBegin(); tEntry != tSlot->value.MemberEnd(); ++tEntry)
        {
            if (tEntry->value.IsObject())
            {
                pTarget[tEntry->name.GetString()] = readRuleValue(tEntry->value, pGlobal ? *pGlobal : AttachmentRule());
            }
        }
    }

    template <typename Writer>
    void writeRule(Writer& pWriter, const AttachmentRule& pRule, bool pIsWeapon)
    {
        auto tWriteVector = [&pWriter](const char* pName, const glm::vec3& pValue)
        {
            pWriter.Key(pName);
            pWriter.StartArray();
            pWriter.Double(pValue.x);
            pWriter.Double(pValue.y);
            pWriter.Double(pValue.z);
            pWriter.EndArray();
        };

        pWriter.StartObject();
        pWriter.Key("joint");
        pWriter.String(pRule.joint.c_str());

        if (pIsWeapon)
        {
            pWriter.Key("length");
            pWriter.Double(pRule.length);
            pWriter.Key("grip");
            pWriter.Double(pRule.grip);
        }
        else
        {
            pWriter.Key("widthScale");
            pWriter.Double(pRule.widthScale);
        }

        tWriteVector("rotation", pRule.rotationDegrees);
        tWriteVector("offset", pRule.offset);
        pWriter.EndObject();
    }

    template <typename Writer>
    void writeOverrides(Writer& pWriter, const char* pSlotName, const std::map<std::string, AttachmentRule>& pOverrides, bool pIsWeapon)
    {
        pWriter.Key(pSlotName);
        pWriter.StartObject();

        for (const auto& [tPath, tRule] : pOverrides)
        {
            pWriter.Key(tPath.c_str());
            writeRule(pWriter, tRule, pIsWeapon);
        }

        pWriter.EndObject();
    }

    glm::mat4 toRotation(const glm::vec3& pDegrees)
    {
        glm::mat4 tRotation(1.0f);
        tRotation = glm::rotate(tRotation, glm::radians(pDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        tRotation = glm::rotate(tRotation, glm::radians(pDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        tRotation = glm::rotate(tRotation, glm::radians(pDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return tRotation;
    }
}

bool EquipmentAttachments::load(const std::string& pPath, std::string& pError)
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
        pError = "invalid JSON in " + pPath;
        return false;
    }

    mWeapon = readRule(tDocument, "weapon");
    mHelmet = readRule(tDocument, "helmet");
    readOverrides(tDocument, "weapon", mWeapon, mWeaponOverrides);
    readOverrides(tDocument, "helmet", mHelmet, mHelmetOverrides);
    mCache.clear();
    return true;
}

bool EquipmentAttachments::save(const std::string& pPath, std::string& pError) const
{
    rapidjson::StringBuffer tBuffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> tWriter(tBuffer);
    tWriter.SetIndent(' ', 2);
    tWriter.SetMaxDecimalPlaces(4);

    tWriter.StartObject();

    if (mWeapon)
    {
        tWriter.Key("weapon");
        writeRule(tWriter, *mWeapon, true);
    }

    if (mHelmet)
    {
        tWriter.Key("helmet");
        writeRule(tWriter, *mHelmet, false);
    }

    if (!mWeaponOverrides.empty() || !mHelmetOverrides.empty())
    {
        tWriter.Key("overrides");
        tWriter.StartObject();

        if (!mWeaponOverrides.empty())
        {
            writeOverrides(tWriter, "weapon", mWeaponOverrides, true);
        }

        if (!mHelmetOverrides.empty())
        {
            writeOverrides(tWriter, "helmet", mHelmetOverrides, false);
        }

        tWriter.EndObject();
    }

    tWriter.EndObject();

    std::ofstream tFile(pPath, std::ios::binary | std::ios::trunc);

    if (!tFile)
    {
        pError = "cannot write " + pPath;
        return false;
    }

    tFile << tBuffer.GetString() << '\n';
    return true;
}

AttachmentRule* EquipmentAttachments::findRule(EquipSlotType pSlot)
{
    std::optional<AttachmentRule>* tRule = pSlot == EquipSlotType::eWeapon ? &mWeapon
                                         : pSlot == EquipSlotType::eHelmet ? &mHelmet
                                         : nullptr;
    return tRule && *tRule ? &**tRule : nullptr;
}

EquipmentAttachments::OverrideMap* EquipmentAttachments::getOverrides(EquipSlotType pSlot)
{
    return pSlot == EquipSlotType::eWeapon ? &mWeaponOverrides
         : pSlot == EquipSlotType::eHelmet ? &mHelmetOverrides
         : nullptr;
}

AttachmentRule* EquipmentAttachments::findOverride(EquipSlotType pSlot, const std::string& pModelPath)
{
    OverrideMap* tOverrides = getOverrides(pSlot);

    if (!tOverrides)
    {
        return nullptr;
    }

    auto tIterator = tOverrides->find(pModelPath);
    return tIterator != tOverrides->end() ? &tIterator->second : nullptr;
}

AttachmentRule* EquipmentAttachments::createOverride(EquipSlotType pSlot, const std::string& pModelPath)
{
    OverrideMap* tOverrides = getOverrides(pSlot);
    AttachmentRule* tGlobal = findRule(pSlot);

    if (!tOverrides || !tGlobal || pModelPath.empty())
    {
        return nullptr;
    }

    mCache.clear();
    return &tOverrides->emplace(pModelPath, *tGlobal).first->second;
}

void EquipmentAttachments::removeOverride(EquipSlotType pSlot, const std::string& pModelPath)
{
    if (OverrideMap* tOverrides = getOverrides(pSlot))
    {
        tOverrides->erase(pModelPath);
        mCache.clear();
    }
}

glm::vec3 EquipmentAttachments::flipRotation(const glm::vec3& pDegrees)
{
    glm::mat4 tFlipped = toRotation(pDegrees) * glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    float tPitch = std::asin(std::clamp(-tFlipped[2][1], -1.0f, 1.0f));
    float tYaw = std::atan2(tFlipped[2][0], tFlipped[2][2]);
    float tRoll = std::atan2(tFlipped[0][1], tFlipped[1][1]);
    return glm::degrees(glm::vec3(tPitch, tYaw, tRoll));
}

const AttachmentRule* EquipmentAttachments::resolveRule(EquipSlotType pSlot, const std::string& pModelPath) const
{
    const OverrideMap* tOverrides = pSlot == EquipSlotType::eWeapon ? &mWeaponOverrides
                                  : pSlot == EquipSlotType::eHelmet ? &mHelmetOverrides
                                  : nullptr;

    if (tOverrides)
    {
        auto tIterator = tOverrides->find(pModelPath);

        if (tIterator != tOverrides->end())
        {
            return &tIterator->second;
        }
    }

    const std::optional<AttachmentRule>& tGlobal = pSlot == EquipSlotType::eWeapon ? mWeapon : mHelmet;
    return pSlot == EquipSlotType::eWeapon || pSlot == EquipSlotType::eHelmet ? (tGlobal ? &*tGlobal : nullptr) : nullptr;
}

void EquipmentAttachments::invalidate()
{
    mCache.clear();
}

const EquipmentAttachments::Placement* EquipmentAttachments::find(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem, const std::string& pModelPath)
{
    auto tKey = std::make_tuple(pSlot, &pCharacter, &pItem);
    auto tIterator = mCache.find(tKey);

    if (tIterator == mCache.end())
    {
        tIterator = mCache.emplace(tKey, compute(pSlot, pCharacter, pItem, pModelPath)).first;
    }

    return tIterator->second ? &*tIterator->second : nullptr;
}

std::optional<EquipmentAttachments::Placement> EquipmentAttachments::compute(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem, const std::string& pModelPath) const
{
    const AttachmentRule* tRule = resolveRule(pSlot, pModelPath);

    if (!tRule)
    {
        return std::nullopt;
    }

    const Skeleton& tSkeleton = pCharacter.getSkeleton();
    int tJoint = tSkeleton.findJointByBaseName(tRule->joint);

    if (tJoint < 0)
    {
        return std::nullopt;
    }

    const SkinnedModel::Bounds& tItemBounds = pItem.getBounds();
    glm::vec3 tItemSize = glm::max(tItemBounds.getSize(), glm::vec3(0.001f));
    glm::mat4 tRotation = toRotation(tRule->rotationDegrees);
    glm::mat4 tTransform(1.0f);

    if (pSlot == EquipSlotType::eWeapon)
    {
        std::vector<glm::mat4> tPalette;
        std::vector<glm::mat4> tGlobals;
        tSkeleton.computePalette(tSkeleton.getBindPose(), tPalette, tGlobals);

        float tScale = tRule->length / tItemSize.y;
        glm::vec3 tGrip(tItemBounds.getCenter().x, tItemBounds.min.y + tItemSize.y * tRule->grip, tItemBounds.getCenter().z);

        tTransform = tGlobals[static_cast<size_t>(tJoint)] * glm::translate(glm::mat4(1.0f), tRule->offset);
        tTransform = tTransform * tRotation;
        tTransform = glm::scale(tTransform, glm::vec3(tScale));
        tTransform = glm::translate(tTransform, -tGrip);
    }
    else
    {
        if (!pCharacter.getJointBounds(tJoint))
        {
            return std::nullopt;
        }

        const SkinnedModel::Bounds& tJointBounds = *pCharacter.getJointBounds(tJoint);
        glm::vec3 tHeadSize = tJointBounds.getSize();
        float tScale = tHeadSize.x * tRule->widthScale / tItemSize.x;
        glm::vec3 tHeadTop(tJointBounds.getCenter().x, tJointBounds.max.y, tJointBounds.getCenter().z);
        glm::vec3 tItemTop(tItemBounds.getCenter().x, tItemBounds.max.y, tItemBounds.getCenter().z);

        tTransform = glm::translate(glm::mat4(1.0f), tHeadTop + tRule->offset);
        tTransform = tTransform * tRotation;
        tTransform = glm::scale(tTransform, glm::vec3(tScale));
        tTransform = glm::translate(tTransform, -tItemTop);
    }

    return Placement{ tJoint, glm::inverse(pCharacter.getBindPalette()[static_cast<size_t>(tJoint)]) * tTransform };
}
