#ifndef LAKOT_SKINNED_MODEL_H
#define LAKOT_SKINNED_MODEL_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AnimationClip.h"
#include "Texture.h"
#include "Skeleton.h"

#include "../render/Renderable.h"

namespace lakot
{

class AssetManager;

class SkinnedModel final : public Renderable
{
public:
    static constexpr size_t kMaxJoints = 64;

    struct Bounds
    {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};

        glm::vec3 getSize() const { return max - min; }
        glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    };

    static std::unique_ptr<SkinnedModel> load(const std::string& pPath, AssetManager& pAssets, std::string& pError);

    ~SkinnedModel() override;

    void initialize() override;
    void deinitialize() override;

    const Skeleton& getSkeleton() const;
    const AnimationClip* findClip(const std::string& pName) const;
    const std::vector<AnimationClip>& getClips() const;

    // Bind pose, in model space.
    const Bounds& getBounds() const;
    float getHeight() const;

    // Bind pose, model space: the vertices this joint moves the most; empty if it moves none.
    const std::optional<Bounds>& getJointBounds(int pJoint) const;

    const std::vector<glm::mat4>& getBindPalette() const;

    void draw(unsigned int pTextureUnit) const;

private:
    SkinnedModel();

    Skeleton mSkeleton;
    std::vector<AnimationClip> mClips;
    std::shared_ptr<Texture> mTexture;

    unsigned int mIndexCount{0};
    Bounds mBounds;
    std::vector<std::optional<Bounds>> mJointBounds;
    std::vector<glm::mat4> mBindPalette;
};

}

#endif
