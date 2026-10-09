#include "EquipmentAttachments.h"

#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "../../graphics/model/SkinnedModel.h"

using namespace lakot;

namespace
{
    glm::vec3 getVector(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);

        if (tMember == pObject.MemberEnd() || !tMember->value.IsArray() || tMember->value.Size() != 3)
        {
            return glm::vec3(0.0f);
        }

        return glm::vec3(tMember->value[0].GetFloat(), tMember->value[1].GetFloat(), tMember->value[2].GetFloat());
    }

    float getFloat(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsNumber() ? tMember->value.GetFloat() : 0.0f;
    }

    std::optional<AttachmentRule> readRule(const rapidjson::Document& pDocument, const char* pName)
    {
        auto tMember = pDocument.FindMember(pName);

        if (tMember == pDocument.MemberEnd() || !tMember->value.IsObject())
        {
            return std::nullopt;
        }

        const rapidjson::Value& tRule = tMember->value;
        AttachmentRule tResult;
        tResult.joint = tRule.HasMember("joint") && tRule["joint"].IsString() ? tRule["joint"].GetString() : "";
        tResult.length = getFloat(tRule, "length");
        tResult.grip = getFloat(tRule, "grip");
        tResult.widthScale = getFloat(tRule, "widthScale");
        tResult.rotationDegrees = getVector(tRule, "rotation");
        tResult.offset = getVector(tRule, "offset");
        return tResult;
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
    mCache.clear();
    return true;
}

bool EquipmentAttachments::save(const std::string& pPath, std::string& pError) const
{
    rapidjson::StringBuffer tBuffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> tWriter(tBuffer);
    tWriter.SetIndent(' ', 2);
    tWriter.SetMaxDecimalPlaces(4);

    auto tWriteVector = [&tWriter](const char* pName, const glm::vec3& pValue)
    {
        tWriter.Key(pName);
        tWriter.StartArray();
        tWriter.Double(pValue.x);
        tWriter.Double(pValue.y);
        tWriter.Double(pValue.z);
        tWriter.EndArray();
    };

    tWriter.StartObject();

    if (mWeapon)
    {
        tWriter.Key("weapon");
        tWriter.StartObject();
        tWriter.Key("joint");
        tWriter.String(mWeapon->joint.c_str());
        tWriter.Key("length");
        tWriter.Double(mWeapon->length);
        tWriter.Key("grip");
        tWriter.Double(mWeapon->grip);
        tWriteVector("rotation", mWeapon->rotationDegrees);
        tWriteVector("offset", mWeapon->offset);
        tWriter.EndObject();
    }

    if (mHelmet)
    {
        tWriter.Key("helmet");
        tWriter.StartObject();
        tWriter.Key("joint");
        tWriter.String(mHelmet->joint.c_str());
        tWriter.Key("widthScale");
        tWriter.Double(mHelmet->widthScale);
        tWriteVector("rotation", mHelmet->rotationDegrees);
        tWriteVector("offset", mHelmet->offset);
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

void EquipmentAttachments::invalidate()
{
    mCache.clear();
}

const EquipmentAttachments::Placement* EquipmentAttachments::find(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem)
{
    auto tKey = std::make_tuple(pSlot, &pCharacter, &pItem);
    auto tIterator = mCache.find(tKey);

    if (tIterator == mCache.end())
    {
        tIterator = mCache.emplace(tKey, compute(pSlot, pCharacter, pItem)).first;
    }

    return tIterator->second ? &*tIterator->second : nullptr;
}

std::optional<EquipmentAttachments::Placement> EquipmentAttachments::compute(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem) const
{
    const std::optional<AttachmentRule>& tRule = pSlot == EquipSlotType::eWeapon ? mWeapon
                                               : pSlot == EquipSlotType::eHelmet ? mHelmet
                                               : std::optional<AttachmentRule>();

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
