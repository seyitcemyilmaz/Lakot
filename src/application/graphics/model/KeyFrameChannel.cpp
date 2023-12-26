#include "KeyFrameChannel.h"

KeyFrameChannel::KeyFrameChannel(KeyFrameChannelResource* pKeyFrameChannelResource, Node* pNode)
    : mKeyFrameChannelResource(pKeyFrameChannelResource)
    , mNode(pNode)
{

}

KeyFrameChannelResource* KeyFrameChannel::getKeyFrameChannelResource()
{
    return mKeyFrameChannelResource;
}

Node* KeyFrameChannel::getNode()
{
    return mNode;
}
