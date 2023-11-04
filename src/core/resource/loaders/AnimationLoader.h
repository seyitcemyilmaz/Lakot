#ifndef LAKOT_ANIMATIONLOADER_H
#define LAKOT_ANIMATIONLOADER_H

#include <assimp/scene.h>

#include "../ModelResource.h"
#include "../KeyFrameChannelResource.h"

class AnimationLoader {
private:
	ModelResource* mModelResource;

	const aiAnimation* mAnimation;
	
	void processKeyFrameChannel(aiNodeAnim* pAiKeyFrameChannel, KeyFrameChannelResource* pKeyFrameChannel);

public:
	AnimationLoader(ModelResource* pModelResource, const aiAnimation* pAnimation);
	AnimationResource* load();
};

#endif
