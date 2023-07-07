#ifndef MATERIALRESOURCE_H
#define MATERIALRESOURCE_H

#include "textureresource.h"

class MaterialResource
{
private:
	TextureResource* mDiffuseTexture;
	TextureResource* mNormalTexture;
	TextureResource* mSpecularTexture;
	TextureResource* mEmmisiveTexture;
	TextureResource* mAmbientTexture;

protected:
	void setDiffuseTexture(TextureResource* pDiffuseTexture);
	void setNormalTexture(TextureResource* pNormalTexture);
	void setSpecularTexture(TextureResource* pSpecularTexture);
	void setEmissiveTexture(TextureResource* pEmmisiveTexture);
	void setAmbientTexture(TextureResource* pAmbientTexture);

	friend class MaterialLoader;

public:
	MaterialResource();
};

#endif // MATERIALRESOURCE_H
