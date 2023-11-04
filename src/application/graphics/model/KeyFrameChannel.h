#ifndef LAKOT_KEYFRAMECHANNEL_H
#define LAKOT_KEYFRAMECHANNEL_H

#include "core/resource/KeyFrameChannelResource.h"

#include "Node.h"

class KeyFrameChannel {
private:
	KeyFrameChannelResource* mKeyFrameChannelResource;

	Node* mNode;

	double getScaleFactor(double pLastTimeStamp, double pNextTimeStamp, double pAnimationTime) const;

public:
	KeyFrameChannel(KeyFrameChannelResource* pKeyFrameChannelResource, Node* pNode);

	KeyFrameChannelResource* getKeyFrameChannelResource();

	Node* getNode();
};

#endif
