#ifndef LAKOT_SCENE_H
#define LAKOT_SCENE_H

#include "application/graphics/model/Model.h"
#include "core/helper/camera/CameraManager.h"

#include "Projection.h"

class Scene
{
public:
    virtual ~Scene();
    Scene();

    void update(double pTimeDifference);

    void processKeyboardInputs(double pTimeDifference);
    void processMouseInputs(double pTimeDifference);

    void addModel(Model* pModel);
    bool deleteModel(Model* pModel);

    const std::vector<Model*>& getModels() const;

    Projection* getProjection() const;
    Camera* getCamera() const;

private:
    std::vector<Model*> mModels;

    CameraManager* mCameraManager;

    Projection* mProjection;

    int getModelIndex(Model* pModel);
};

#endif
