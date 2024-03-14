#include "RenderManager.h"

#include <spdlog/spdlog.h>
#include <core/helper/GLMConverter.h>

#include "platform/Platform.h"

#include "core/helper/shader/ShaderManager.h"

RenderManager* RenderManager::mInstance = nullptr;

RenderManager* RenderManager::getInstance()
{
    if (!mInstance)
    {
        mInstance = new RenderManager();
    }

    return mInstance;
}

void RenderManager::renderScene(Scene* pScene)
{
    Projection* tProjection = pScene->getProjection();

    glViewport(tProjection->getX(),
               tProjection->getY(),
               tProjection->getWidth(),
               tProjection->getHeight());

    const glm::mat4& tProjectionMatrix = tProjection->getProjectionMatrix();
    const glm::mat4& tViewMatrix = pScene->getCamera()->getViewMatrix();

    const std::vector<Model*> pModels = pScene->getModels();

    renderModels(pModels, tProjectionMatrix, tViewMatrix);
}

void RenderManager::renderGUI()
{
    // TODO: Add gui render
}

void RenderManager::renderModel(Model* pModel, ShaderProgram* pShaderProgram)
{
    const std::vector<Mesh*> pMeshes = pModel->getMeshes();
    unsigned int tMeshCount = pModel->getMeshCount();

    for (unsigned int i = 0; i < tMeshCount; i++)
    {
        renderMesh(pModel, pMeshes[i], pShaderProgram);
    }
}

void RenderManager::renderMesh(Model* pModel, Mesh* pMesh, ShaderProgram* pShaderProgram)
{
    useMaterial(pModel, pMesh, pShaderProgram);

    if (pMesh->getHasBone() && pModel->getHasActiveAnimation())
    {
        pShaderProgram->setInt("animationType", 1);

        const std::vector<glm::mat4>& tBoneMatrices = pModel->getBoneMatrices();
        pShaderProgram->setMat4Array("boneTransformations", tBoneMatrices.data(), static_cast<unsigned int>(tBoneMatrices.size()));

        pShaderProgram->setMat4("model", pModel->getModelMatrix() * pMesh->getTransformationMatrix());
    }
    else
    {
        pShaderProgram->setInt("animationType", 0);
        pShaderProgram->setMat4("model", pModel->getModelMatrix() * pMesh->getTransformationMatrix());
    }

    glBindVertexArray(pMesh->getMeshResource()->getVAO());
    glDrawElements(GL_TRIANGLES, pMesh->getMeshResource()->getIndiceCount(), GL_UNSIGNED_INT, nullptr);
}

void RenderManager::useMaterial(Model* pModel, Mesh* pMesh, ShaderProgram* pShaderProgram)
{
    MaterialResource* tMaterialResource = pModel->getModelResource()->getMaterialResource(pMesh->getMaterialIndex());

    if (tMaterialResource)
    {
        TextureResource* tDiffuseTexture = tMaterialResource->getDiffuseTexture();

        unsigned int tUnit = 0;

        if (tDiffuseTexture)
        {
            pShaderProgram->setBool("material.hasDiffuseTexture", true);
            pShaderProgram->setTexture("material.diffuseTexture", tUnit++, tDiffuseTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasDiffuseTexture", false);
            pShaderProgram->setVec3("material.diffuseColor", tMaterialResource->getDiffuseColor());
            tUnit++;
        }

        TextureResource* tNormalsTexture = tMaterialResource->getNormalsTexture();

        if (tNormalsTexture)
        {
            pShaderProgram->setBool("material.hasNormalsTexture", true);
            pShaderProgram->setTexture("material.normalsTexture", tUnit++, tNormalsTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasNormalsTexture", false);
            tUnit++;
        }

        TextureResource* tSpecularTexture = tMaterialResource->getSpecularTexture();

        if (tSpecularTexture)
        {
            pShaderProgram->setBool("material.hasSpecularTexture", true);
            pShaderProgram->setTexture("material.specularTexture", tUnit++, tSpecularTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasSpecularTexture", false);
            pShaderProgram->setVec3("material.specularColor", tMaterialResource->getSpecularColor());
            tUnit++;
        }

        TextureResource* tEmissiveTexture = tMaterialResource->getEmissiveTexture();

        if (tEmissiveTexture)
        {
            pShaderProgram->setBool("material.hasEmissiveTexture", true);
            pShaderProgram->setTexture("material.emissiveTexture", tUnit++, tEmissiveTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasEmissiveTexture", false);
            pShaderProgram->setVec3("material.emissiveColor", tMaterialResource->getEmissiveColor());
            tUnit++;
        }

        TextureResource* tAmbientTexture = tMaterialResource->getAmbientTexture();

        if (tAmbientTexture)
        {
            pShaderProgram->setBool("material.hasAmbientTexture", true);
            pShaderProgram->setTexture("material.ambientTexture", tUnit++, tAmbientTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasAmbientTexture", false);
            pShaderProgram->setVec3("material.ambientColor", tMaterialResource->getAmbientColor());
            tUnit++;
        }

        TextureResource* tMetalnessTexture = tMaterialResource->getMetalnessTexture();

        if (tMetalnessTexture)
        {
            pShaderProgram->setBool("material.hasMetalnessTexture", true);
            pShaderProgram->setTexture("material.metalnessTexture", tUnit++, tMetalnessTexture->getTextureId());
        }
        else
        {
            pShaderProgram->setBool("material.hasMetalnessTexture", false);
            tUnit++;
        }
    }
}

void RenderManager::renderModels(const std::vector<Model*> pModels,
                                 const glm::mat4& pProjectionMatrix,
                                 const glm::mat4& pViewMatrix)
{
    ShaderProgram* tShader = ShaderManager::getInstance()->getShaderProgram("model");
    ShaderManager::getInstance()->bindShaderProgram(tShader);

    tShader->setMat4("projection", pProjectionMatrix);
    tShader->setMat4("view", pViewMatrix);

    for (unsigned int i = 0; i < pModels.size(); i++)
    {
        const glm::mat4& tModelMatrix = pModels[i]->getModelMatrix();
        tShader->setMat4("model", tModelMatrix);
        renderModel(pModels[i], tShader);
    }
}

void RenderManager::renderParticles(const std::vector<Particle*>& pParticles, ShaderProgram* pShaderProgram)
{
    // glm::mat4 tProjectionMatrix = getProjectionMatrix();
    // glm::mat4 tViewMatrix = getViewMatrix();

    // ShaderManager::getInstance()->bindShader(pShader);

    // pShader->getShaderVariable(ShaderVariableName::eProjection)->setMat4(tProjectionMatrix);
    // pShader->getShaderVariable(ShaderVariableName::eView)->setMat4(tViewMatrix);

    // for (unsigned int i = 0; i < pParticles.size(); i++)
    // {
    //     Particle* tParticle = pParticles[i];

    //     if (tParticle->getIsActive())
    //     {
    //         const glm::vec3& tPosition = tParticle->getPosition();
    //     }
    // }
}

RenderManager::RenderManager()
    : mParticleRenderer(nullptr)
{

}
