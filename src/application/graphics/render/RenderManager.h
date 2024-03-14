#ifndef LAKOT_RENDERER_H
#define LAKOT_RENDERER_H

#include <glm/glm.hpp>

#include "../model/Model.h"
#include "helper/shader/ShaderProgram.h"
#include "../particle/ParticleRenderer.h"
#include "../scene/Scene.h"

class RenderManager
{
public:
    static RenderManager* getInstance();

    void renderScene(Scene* pScene);

    void renderGUI();

    void renderParticles(const std::vector<Particle*>& pParticles, ShaderProgram* pShaderProgrma);

private:
    static RenderManager* mInstance;

    ParticleRenderer* mParticleRenderer;

    RenderManager();

    void renderModel(Model* pModel, ShaderProgram* pShaderProgram);
    void renderMesh(Model* pModel, Mesh* pMesh, ShaderProgram* pShaderProgram);
    void useMaterial(Model* pModel, Mesh* pMesh, ShaderProgram* pShaderProgram);

    void renderModels(const std::vector<Model*> pModels,
                      const glm::mat4& pProjectionMatrix,
                      const glm::mat4& pViewMatrix);


};

#endif
