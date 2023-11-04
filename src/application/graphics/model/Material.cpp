#include "Material.h"

Material::Material(MaterialResource* pMaterialResource)
	: mMaterialResource(pMaterialResource) { }

MaterialResource* Material::getMaterialResource() {
	return mMaterialResource;
}
