#ifndef LAKOT_MODEL_H
#define LAKOT_MODEL_H

#include "core/resource/ModelResource.h"

#include "Node.h"
#include "Animation.h"
#include "Material.h"
#include "Bone.h"

#include "interface/ITransformable.h"
#include "interface/IAnimated.h"

class Model : public ITransformable, public IAnimated {
private:
    ModelResource* mModelResource;

    Node* mRootNode;

    std::vector<Mesh*> mMeshes;
    std::vector<Bone*> mBones;
    std::vector<Node*> mNodes;
    std::vector<Material*> mMaterials;
    std::vector<Animation*> mAnimations;

    std::vector<Animation*> mActiveAnimations;

protected:
    void calculateBoneTransformations(INode* pNode, const glm::mat4& tParentTransform) override;
    void updateAnimations(double pTimeDifference) override;

    Node* getNode(const std::string& pNodeName);
    friend class ModelFactory;

public:
    Model(ModelResource* pModelResource, Node* pRootNode);

    void playAnimationContinuously(unsigned int pIndex) override;
    void playAnimationOnlyOnce(unsigned int pIndex) override;
    void pauseAnimation(unsigned int pIndex) override;
    void stopAnimation(unsigned int pIndex) override;

    void update(double pTimeDifference);

    ModelResource* getModelResource();

    Node* getRootNode();

    const glm::mat4& getGlobalInverseTransform();

    unsigned int getMeshCount() const;
    unsigned int getNodeCount() const;
    unsigned int getBoneCount() const;
    unsigned int getAnimationCount() const;
    unsigned int getActiveAnimationCount() const;

    bool getHasActiveAnimation() const;

    std::vector<glm::mat4> getBoneMatrices() const;

    const std::vector<Mesh*>& getMeshes();
    const std::vector<Bone*>& getBones();
    const std::vector<Node*>& getNodes();
    const std::vector<Material*>& getMaterials();
};

#endif
