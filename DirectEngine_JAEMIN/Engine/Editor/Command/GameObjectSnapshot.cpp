#include "GameObjectSnapshot.h"

#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/Scene.h"

namespace Engine::Editor
{
    namespace
    {
        std::shared_ptr<Renderer::Material> CloneMaterialInstance(const Renderer::Material& source)
        {
            auto clone = std::make_shared<Renderer::Material>();
            clone->SetName(source.GetName());
            clone->SetVertexShader(source.GetVertexShader());
            clone->SetPixelShader(source.GetPixelShader());
            clone->SetBaseColor(source.GetBaseColor());
            clone->SetRoughness(source.GetRoughness());
            clone->SetMetallic(source.GetMetallic());
            clone->SetBaseColorTexture(source.GetBaseColorTexture());
            clone->SetRoughnessTexture(source.GetRoughnessTexture());
            clone->SetMetallicTexture(source.GetMetallicTexture());
            clone->SetNormalTexture(source.GetNormalTexture());
            clone->SetVertexShaderPath(source.GetVertexShaderPath());
            clone->SetPixelShaderPath(source.GetPixelShaderPath());
            clone->SetBaseColorTexturePath(source.GetBaseColorTexturePath());
            clone->SetRoughnessTexturePath(source.GetRoughnessTexturePath());
            clone->SetMetallicTexturePath(source.GetMetallicTexturePath());
            clone->SetNormalTexturePath(source.GetNormalTexturePath());
            return clone;
        }
    }

    GameObjectSnapshot GameObjectSnapshot::Capture(const Scene::GameObject& object)
    {
        GameObjectSnapshot snapshot;
        snapshot.name = object.GetName();
        snapshot.transform = object.GetTransform();

        if (const Scene::MeshRendererComponent* meshRenderer = object.GetComponent<Scene::MeshRendererComponent>())
        {
            snapshot.meshRenderer.enabled = true;
            snapshot.meshRenderer.mesh = meshRenderer->GetMesh();
            snapshot.meshRenderer.material = meshRenderer->GetMaterial();
            snapshot.meshRenderer.meshPath = meshRenderer->GetMeshPath();
            snapshot.meshRenderer.materialPath = meshRenderer->GetMaterialPath();
            snapshot.meshRenderer.meshGuid = meshRenderer->GetMeshGuid();
            snapshot.meshRenderer.materialGuid = meshRenderer->GetMaterialGuid();
        }

        for (const Physics::ColliderComponent* collider : object.GetComponents<Physics::ColliderComponent>())
        {
            if (!collider)
            {
                continue;
            }

            ColliderSnapshot colliderSnapshot;
            colliderSnapshot.type = collider->type;
            colliderSnapshot.center = collider->center;
            colliderSnapshot.size = collider->size;
            colliderSnapshot.radius = collider->radius;
            colliderSnapshot.isTrigger = collider->isTrigger;
            snapshot.colliders.push_back(colliderSnapshot);
        }

        if (const Scene::LightComponent* light = object.GetComponent<Scene::LightComponent>())
        {
            snapshot.light.enabled = true;
            snapshot.light.type = light->type;
            snapshot.light.color = light->color;
            snapshot.light.intensity = light->intensity;
            snapshot.light.range = light->range;
            snapshot.light.direction = light->direction;
            snapshot.light.innerConeAngle = light->innerConeAngle;
            snapshot.light.outerConeAngle = light->outerConeAngle;
            snapshot.light.ambientColor = light->ambientColor;
        }

        if (const Scene::PostProcessComponent* postProcess = object.GetComponent<Scene::PostProcessComponent>())
        {
            snapshot.postProcess.enabledComponent = true;
            snapshot.postProcess.enabled = postProcess->enabled;
            snapshot.postProcess.effect = postProcess->effect;
            snapshot.postProcess.grayscaleEnabled = postProcess->grayscaleEnabled;
            snapshot.postProcess.vignetteEnabled = postProcess->vignetteEnabled;
            snapshot.postProcess.toneMappingEnabled = postProcess->toneMappingEnabled;
            snapshot.postProcess.colorAdjustEnabled = postProcess->colorAdjustEnabled;
            snapshot.postProcess.grayscaleIntensity = postProcess->grayscaleIntensity;
            snapshot.postProcess.exposure = postProcess->exposure;
            snapshot.postProcess.gamma = postProcess->gamma;
            snapshot.postProcess.vignetteStrength = postProcess->vignetteStrength;
            snapshot.postProcess.vignetteRadius = postProcess->vignetteRadius;
            snapshot.postProcess.vignetteSoftness = postProcess->vignetteSoftness;
            snapshot.postProcess.brightness = postProcess->brightness;
            snapshot.postProcess.contrast = postProcess->contrast;
            snapshot.postProcess.saturation = postProcess->saturation;
        }

        if (const Scene::PlayerStartComponent* playerStart = object.GetComponent<Scene::PlayerStartComponent>())
        {
            snapshot.playerStart.enabled = true;
            snapshot.playerStart.playerHeight = playerStart->playerHeight;
            snapshot.playerStart.moveSpeed = playerStart->moveSpeed;
            snapshot.playerStart.fastMoveMultiplier = playerStart->fastMoveMultiplier;
            snapshot.playerStart.mouseSensitivity = playerStart->mouseSensitivity;
        }

        return snapshot;
    }

    Scene::GameObject& GameObjectSnapshot::Restore(Scene::Scene& scene) const
    {
        Scene::GameObject& object = scene.CreateGameObject(name);
        object.GetTransform() = transform;

        if (meshRenderer.enabled)
        {
            Scene::MeshRendererComponent& component = object.AddComponent<Scene::MeshRendererComponent>();
            component.SetMesh(meshRenderer.mesh);
            component.SetMaterial(meshRenderer.cloneMaterialOnRestore && meshRenderer.material
                ? CloneMaterialInstance(*meshRenderer.material)
                : meshRenderer.material);
            component.SetMeshPath(meshRenderer.meshPath);
            component.SetMaterialPath(meshRenderer.cloneMaterialOnRestore ? std::wstring() : meshRenderer.materialPath);
            component.SetMeshGuid(meshRenderer.meshGuid);
            component.SetMaterialGuid(meshRenderer.cloneMaterialOnRestore ? std::string() : meshRenderer.materialGuid);
        }

        for (const ColliderSnapshot& colliderSnapshot : colliders)
        {
            Physics::ColliderComponent& collider = object.AddComponent<Physics::ColliderComponent>();
            collider.type = colliderSnapshot.type;
            collider.center = colliderSnapshot.center;
            collider.size = colliderSnapshot.size;
            collider.radius = colliderSnapshot.radius;
            collider.isTrigger = colliderSnapshot.isTrigger;
        }

        if (light.enabled)
        {
            Scene::LightComponent& component = object.AddComponent<Scene::LightComponent>();
            component.type = light.type;
            component.color = light.color;
            component.intensity = light.intensity;
            component.range = light.range;
            component.direction = light.direction;
            component.innerConeAngle = light.innerConeAngle;
            component.outerConeAngle = light.outerConeAngle;
            component.ambientColor = light.ambientColor;
        }

        if (postProcess.enabledComponent)
        {
            Scene::PostProcessComponent& component = object.AddComponent<Scene::PostProcessComponent>();
            component.enabled = postProcess.enabled;
            component.effect = postProcess.effect;
            component.grayscaleEnabled = postProcess.grayscaleEnabled;
            component.vignetteEnabled = postProcess.vignetteEnabled;
            component.toneMappingEnabled = postProcess.toneMappingEnabled;
            component.colorAdjustEnabled = postProcess.colorAdjustEnabled;
            component.grayscaleIntensity = postProcess.grayscaleIntensity;
            component.exposure = postProcess.exposure;
            component.gamma = postProcess.gamma;
            component.vignetteStrength = postProcess.vignetteStrength;
            component.vignetteRadius = postProcess.vignetteRadius;
            component.vignetteSoftness = postProcess.vignetteSoftness;
            component.brightness = postProcess.brightness;
            component.contrast = postProcess.contrast;
            component.saturation = postProcess.saturation;
        }

        if (playerStart.enabled)
        {
            Scene::PlayerStartComponent& component = object.AddComponent<Scene::PlayerStartComponent>();
            component.playerHeight = playerStart.playerHeight;
            component.moveSpeed = playerStart.moveSpeed;
            component.fastMoveMultiplier = playerStart.fastMoveMultiplier;
            component.mouseSensitivity = playerStart.mouseSensitivity;
        }

        return object;
    }
}
