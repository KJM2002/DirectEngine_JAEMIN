#include "SceneCloner.h"

#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Light.h"
#include "Engine/Scene/LightComponent.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/PostProcessComponent.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneIDs.h"

#include <unordered_map>

namespace Engine::Scene
{
    namespace
    {
        void CopyComponents(const GameObject& source, GameObject& target)
        {
            if (const MeshRendererComponent* sourceMeshRenderer = source.GetComponent<MeshRendererComponent>())
            {
                MeshRendererComponent& meshRenderer = target.AddComponent<MeshRendererComponent>();
                meshRenderer.SetMesh(sourceMeshRenderer->GetMesh());
                meshRenderer.SetMaterial(sourceMeshRenderer->GetMaterial());
                meshRenderer.SetMeshPath(sourceMeshRenderer->GetMeshPath());
                meshRenderer.SetMaterialPath(sourceMeshRenderer->GetMaterialPath());
                meshRenderer.SetMeshGuid(sourceMeshRenderer->GetMeshGuid());
                meshRenderer.SetMaterialGuid(sourceMeshRenderer->GetMaterialGuid());
            }

            if (const LightComponent* sourceLight = source.GetComponent<LightComponent>())
            {
                LightComponent& light = target.AddComponent<LightComponent>();
                light.type = sourceLight->type;
                light.color = sourceLight->color;
                light.intensity = sourceLight->intensity;
                light.range = sourceLight->range;
                light.direction = sourceLight->direction;
                light.innerConeAngle = sourceLight->innerConeAngle;
                light.outerConeAngle = sourceLight->outerConeAngle;
                light.ambientColor = sourceLight->ambientColor;
            }

            for (const Physics::ColliderComponent* sourceCollider : source.GetComponents<Physics::ColliderComponent>())
            {
                if (!sourceCollider)
                {
                    continue;
                }

                Physics::ColliderComponent& collider = target.AddComponent<Physics::ColliderComponent>();
                collider.type = sourceCollider->type;
                collider.center = sourceCollider->center;
                collider.rotation = sourceCollider->rotation;
                collider.size = sourceCollider->size;
                collider.radius = sourceCollider->radius;
                collider.isTrigger = sourceCollider->isTrigger;
            }

            if (const PostProcessComponent* sourcePostProcess = source.GetComponent<PostProcessComponent>())
            {
                PostProcessComponent& postProcess = target.AddComponent<PostProcessComponent>();
                postProcess.enabled = sourcePostProcess->enabled;
                postProcess.effect = sourcePostProcess->effect;
                postProcess.grayscaleEnabled = sourcePostProcess->grayscaleEnabled;
                postProcess.vignetteEnabled = sourcePostProcess->vignetteEnabled;
                postProcess.toneMappingEnabled = sourcePostProcess->toneMappingEnabled;
                postProcess.colorAdjustEnabled = sourcePostProcess->colorAdjustEnabled;
                postProcess.grayscaleIntensity = sourcePostProcess->grayscaleIntensity;
                postProcess.exposure = sourcePostProcess->exposure;
                postProcess.gamma = sourcePostProcess->gamma;
                postProcess.vignetteStrength = sourcePostProcess->vignetteStrength;
                postProcess.vignetteRadius = sourcePostProcess->vignetteRadius;
                postProcess.vignetteSoftness = sourcePostProcess->vignetteSoftness;
                postProcess.brightness = sourcePostProcess->brightness;
                postProcess.contrast = sourcePostProcess->contrast;
                postProcess.saturation = sourcePostProcess->saturation;
            }

            if (const PlayerStartComponent* sourcePlayerStart = source.GetComponent<PlayerStartComponent>())
            {
                PlayerStartComponent& playerStart = target.AddComponent<PlayerStartComponent>();
                playerStart.playerHeight = sourcePlayerStart->playerHeight;
                playerStart.moveSpeed = sourcePlayerStart->moveSpeed;
                playerStart.fastMoveMultiplier = sourcePlayerStart->fastMoveMultiplier;
                playerStart.mouseSensitivity = sourcePlayerStart->mouseSensitivity;
            }
        }
    }

    std::unique_ptr<Scene> SceneCloner::Clone(const Scene& source)
    {
        auto scene = std::make_unique<Scene>();
        CloneInto(source, *scene);
        return scene;
    }

    void SceneCloner::CloneInto(const Scene& source, Scene& target)
    {
        target.Clear();

        for (const std::string& folder : source.GetOutlinerFolders())
        {
            target.AddOutlinerFolder(folder);
        }

        if (source.GetActiveCamera())
        {
            target.SetActiveCamera(std::make_shared<Camera>(*source.GetActiveCamera()));
        }

        if (source.GetDirectionalLight())
        {
            target.SetDirectionalLight(std::make_shared<DirectionalLight>(*source.GetDirectionalLight()));
        }

        std::unordered_map<ObjectID, ObjectID> remappedObjectIds;
        for (const std::unique_ptr<GameObject>& sourceObject : source.GetGameObjects())
        {
            if (!sourceObject)
            {
                continue;
            }

            GameObject& targetObject = target.CreateGameObject(sourceObject->GetName());
            remappedObjectIds[sourceObject->GetID()] = targetObject.GetID();
            targetObject.SetOutlinerFolder(sourceObject->GetOutlinerFolder());
            targetObject.GetTransform() = sourceObject->GetTransform();
            CopyComponents(*sourceObject, targetObject);
        }

        const std::vector<std::unique_ptr<GameObject>>& sourceObjects = source.GetGameObjects();
        const std::vector<std::unique_ptr<GameObject>>& targetObjects = target.GetGameObjects();
        for (std::size_t index = 0; index < sourceObjects.size() && index < targetObjects.size(); ++index)
        {
            const GameObject* sourceObject = sourceObjects[index].get();
            GameObject* targetObject = targetObjects[index].get();
            if (!sourceObject || !targetObject)
            {
                continue;
            }

            const auto parentIt = remappedObjectIds.find(sourceObject->GetParentID());
            targetObject->SetParentID(parentIt == remappedObjectIds.end() ? InvalidObjectID : parentIt->second);
            targetObject->ClearChildren();
            for (ObjectID childId : sourceObject->GetChildren())
            {
                const auto childIt = remappedObjectIds.find(childId);
                if (childIt != remappedObjectIds.end())
                {
                    targetObject->AddChildID(childIt->second);
                }
            }
        }

        target.ClearDirty();
    }
}
