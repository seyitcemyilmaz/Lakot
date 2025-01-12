#ifndef LAKOT_AMODEL_H
#define LAKOT_AMODEL_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AMesh;
class ANode;
class ABone;
class ATexture;
class AMaterial;

class AModel : public Object
{
public:
    virtual ~AModel() override;
    AModel();

    virtual const std::string& getPath() const = 0;

    virtual const std::vector<AMesh*> getMeshes() const = 0;
    virtual const std::vector<ANode*> getNodes() const = 0;
    virtual const std::vector<ABone*> getBones() const = 0;

    virtual const std::vector<ATexture*> getTextures() const = 0;
    virtual const std::vector<AMaterial*> getMaterials() const = 0;

    virtual ANode* getRootNode() const = 0;
};

}

#endif
