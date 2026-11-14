#ifndef LAKOT_MOTION_LIBRARY_H
#define LAKOT_MOTION_LIBRARY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AnimationClip.h"
#include "ClipDefinition.h"

namespace lakot
{

class AssetManager;
class SkinnedModel;

class MotionLibrary
{
public:
    explicit MotionLibrary(AssetManager& pAssets);

    bool load(const std::string& pManifestPath, std::string& pError);

    bool hasClip(const std::string& pName) const;
    const AnimationClip* getClip(const SkinnedModel& pTarget, const std::string& pName);

    const ClipDefinition* findClipDefinition(const std::string& pName) const;
    const std::unordered_map<std::string, std::string>& getRoles() const;
    const std::string* findSkillClip(const std::string& pSkillKey) const;

private:
    struct Source
    {
        std::shared_ptr<SkinnedModel> model;
        const AnimationClip* clip;
    };

    AssetManager& mAssets;
    std::unordered_map<std::string, ClipDefinition> mDefinitions;
    std::unordered_map<std::string, std::string> mRoles;
    std::unordered_map<std::string, std::string> mSkillClips;
    std::unordered_map<std::string, Source> mSources;
    std::unordered_map<const Skeleton*, std::unordered_map<std::string, std::unique_ptr<AnimationClip>>> mRetargeted;
};

}

#endif
