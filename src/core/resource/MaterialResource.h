#ifndef MATERIALRESOURCE_H
#define MATERIALRESOURCE_H

#include "TextureResource.h"

class MaterialResource {
private:
	TextureResource* mDiffuseTexture = nullptr;
	TextureResource* mNormalTexture = nullptr;
	TextureResource* mSpecularTexture = nullptr;
	TextureResource* mEmmisiveTexture = nullptr;
	TextureResource* mAmbientTexture = nullptr;

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