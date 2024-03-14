#include "CameraElement.h"

#include <imgui.h>

#include "application/graphics/scene/Scene.h"

CameraElement::~CameraElement()
{

}

CameraElement::CameraElement()
    : IGuiElement()
{

}

void CameraElement::render(void* pObject)
{
    Scene* tScene = reinterpret_cast<Scene*>(pObject);

    Camera* tCamera = tScene->getCamera();

    ImGui::Begin("Camera");

    glm::vec3 tPreviousPosition = tCamera->getPosition();
    glm::vec3 tNewPosition = tPreviousPosition;
    ImGui::DragFloat3("Position", (float*)&tNewPosition, 0.01f);

    if (tPreviousPosition != tNewPosition)
    {
        tCamera->setPosition(tNewPosition);
    }

    ImGui::End();
}
