#include "GameObjectSnapshot.h"

#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/Scene.h"

#include <cmath>

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
            clone->SetNormalStrength(source.GetNormalStrength());
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

        bool Different(float a, float b, float epsilon)
        {
            return std::abs(a - b) > epsilon;
        }

        bool Different(const Math::Vector3& a, const Math::Vector3& b, float epsilon)
        {
            return Different(a.x, b.x, epsilon) || Different(a.y, b.y, epsilon) || Different(a.z, b.z, epsilon);
        }

        bool Different(const Math::Vector4& a, const Math::Vector4& b, float epsilon)
        {
            return Different(a.x, b.x, epsilon) || Different(a.y, b.y, epsilon) || Different(a.z, b.z, epsilon) || Different(a.w, b.w, epsilon);
        }

        bool Different(const Math::Transform& a, const Math::Transform& b, float epsilon)
        {
            return Different(a.position, b.position, epsilon) || Different(a.rotation, b.rotation, epsilon) || Different(a.scale, b.scale, epsilon);
        }

        void ApplyMaterialState(Scene::MeshRendererComponent& component, const MeshRendererSnapshot& snapshot)
        {
            std::shared_ptr<Renderer::Material> material = component.GetMaterial();
            if (!material || !snapshot.hasMaterialState)
            {
                return;
            }

            material->SetBaseColor(snapshot.materialBaseColor);
            material->SetRoughness(snapshot.materialRoughness);
            material->SetMetallic(snapshot.materialMetallic);
            material->SetNormalStrength(snapshot.materialNormalStrength);
            material->SetBaseColorTexture(snapshot.baseColorTexture);
            material->SetRoughnessTexture(snapshot.roughnessTexture);
            material->SetMetallicTexture(snapshot.metallicTexture);
            material->SetNormalTexture(snapshot.normalTexture);
            material->SetBaseColorTexturePath(snapshot.baseColorTexturePath);
            material->SetRoughnessTexturePath(snapshot.roughnessTexturePath);
            material->SetMetallicTexturePath(snapshot.metallicTexturePath);
            material->SetNormalTexturePath(snapshot.normalTexturePath);
        }
    }

    GameObjectSnapshot GameObjectSnapshot::Capture(const Scene::GameObject& object)
    {
        GameObjectSnapshot snapshot;
        snapshot.id = object.GetID();
        snapshot.parentId = object.GetParentID();
        snapshot.children = object.GetChildren();
        snapshot.name = object.GetName();
        snapshot.outlinerFolder = object.GetOutlinerFolder();
        snapshot.transform = object.GetTransform();

        if (const Scene::MeshRendererComponent* meshRenderer = object.GetComponent<Scene::MeshRendererComponent>())
        {
            snapshot.meshRenderer.componentId = meshRenderer->GetID();
            snapshot.meshRenderer.enabled = true;
            snapshot.meshRenderer.mesh = meshRenderer->GetMesh();
            snapshot.meshRenderer.material = meshRenderer->GetMaterial();
            snapshot.meshRenderer.meshPath = meshRenderer->GetMeshPath();
            snapshot.meshRenderer.materialPath = meshRenderer->GetMaterialPath();
            snapshot.meshRenderer.meshGuid = meshRenderer->GetMeshGuid();
            snapshot.meshRenderer.materialGuid = meshRenderer->GetMaterialGuid();
            if (const Renderer::Material* material = meshRenderer->GetMaterial().get())
            {
                snapshot.meshRenderer.hasMaterialState = true;
                snapshot.meshRenderer.materialBaseColor = material->GetBaseColor();
                snapshot.meshRenderer.materialRoughness = material->GetRoughness();
                snapshot.meshRenderer.materialMetallic = material->GetMetallic();
                snapshot.meshRenderer.materialNormalStrength = material->GetNormalStrength();
                snapshot.meshRenderer.baseColorTexture = material->GetBaseColorTexture();
                snapshot.meshRenderer.roughnessTexture = material->GetRoughnessTexture();
                snapshot.meshRenderer.metallicTexture = material->GetMetallicTexture();
                snapshot.meshRenderer.normalTexture = material->GetNormalTexture();
                snapshot.meshRenderer.baseColorTexturePath = material->GetBaseColorTexturePath();
                snapshot.meshRenderer.roughnessTexturePath = material->GetRoughnessTexturePath();
                snapshot.meshRenderer.metallicTexturePath = material->GetMetallicTexturePath();
                snapshot.meshRenderer.normalTexturePath = material->GetNormalTexturePath();
            }
        }

        for (const Physics::ColliderComponent* collider : object.GetComponents<Physics::ColliderComponent>())
        {
            if (!collider)
            {
                continue;
            }

            ColliderSnapshot colliderSnapshot;
            colliderSnapshot.componentId = collider->GetID();
            colliderSnapshot.type = collider->type;
            colliderSnapshot.center = collider->center;
            colliderSnapshot.rotation = collider->rotation;
            colliderSnapshot.size = collider->size;
            colliderSnapshot.radius = collider->radius;
            colliderSnapshot.isTrigger = collider->isTrigger;
            snapshot.colliders.push_back(colliderSnapshot);
        }

        if (const Scene::LightComponent* light = object.GetComponent<Scene::LightComponent>())
        {
            snapshot.light.componentId = light->GetID();
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
            snapshot.postProcess.componentId = postProcess->GetID();
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
            snapshot.playerStart.componentId = playerStart->GetID();
            snapshot.playerStart.enabled = true;
            snapshot.playerStart.playerHeight = playerStart->playerHeight;
            snapshot.playerStart.moveSpeed = playerStart->moveSpeed;
            snapshot.playerStart.fastMoveMultiplier = playerStart->fastMoveMultiplier;
            snapshot.playerStart.mouseSensitivity = playerStart->mouseSensitivity;
        }

        return snapshot;
    }

    bool GameObjectSnapshot::IsDifferent(const GameObjectSnapshot& a, const GameObjectSnapshot& b, float epsilon)
    {
        if (a.id != b.id || a.parentId != b.parentId || a.children != b.children
            || a.name != b.name || a.outlinerFolder != b.outlinerFolder || Different(a.transform, b.transform, epsilon))
        {
            return true;
        }

        const MeshRendererSnapshot& am = a.meshRenderer;
        const MeshRendererSnapshot& bm = b.meshRenderer;
        if (am.componentId != bm.componentId
            || am.enabled != bm.enabled
            || am.mesh != bm.mesh
            || am.material != bm.material
            || am.meshPath != bm.meshPath
            || am.materialPath != bm.materialPath
            || am.meshGuid != bm.meshGuid
            || am.materialGuid != bm.materialGuid
            || am.hasMaterialState != bm.hasMaterialState
            || Different(am.materialBaseColor, bm.materialBaseColor, epsilon)
            || Different(am.materialRoughness, bm.materialRoughness, epsilon)
            || Different(am.materialMetallic, bm.materialMetallic, epsilon)
            || Different(am.materialNormalStrength, bm.materialNormalStrength, epsilon)
            || am.baseColorTexture != bm.baseColorTexture
            || am.roughnessTexture != bm.roughnessTexture
            || am.metallicTexture != bm.metallicTexture
            || am.normalTexture != bm.normalTexture
            || am.baseColorTexturePath != bm.baseColorTexturePath
            || am.roughnessTexturePath != bm.roughnessTexturePath
            || am.metallicTexturePath != bm.metallicTexturePath
            || am.normalTexturePath != bm.normalTexturePath)
        {
            return true;
        }

        if (a.colliders.size() != b.colliders.size())
        {
            return true;
        }
        for (std::size_t i = 0; i < a.colliders.size(); ++i)
        {
            const ColliderSnapshot& ac = a.colliders[i];
            const ColliderSnapshot& bc = b.colliders[i];
            if (ac.componentId != bc.componentId || ac.type != bc.type || Different(ac.center, bc.center, epsilon) || Different(ac.rotation, bc.rotation, epsilon) || Different(ac.size, bc.size, epsilon)
                || Different(ac.radius, bc.radius, epsilon) || ac.isTrigger != bc.isTrigger)
            {
                return true;
            }
        }

        const LightSnapshot& al = a.light;
        const LightSnapshot& bl = b.light;
        if (al.componentId != bl.componentId || al.enabled != bl.enabled || al.type != bl.type || Different(al.color, bl.color, epsilon)
            || Different(al.intensity, bl.intensity, epsilon) || Different(al.range, bl.range, epsilon)
            || Different(al.direction, bl.direction, epsilon) || Different(al.innerConeAngle, bl.innerConeAngle, epsilon)
            || Different(al.outerConeAngle, bl.outerConeAngle, epsilon) || Different(al.ambientColor, bl.ambientColor, epsilon))
        {
            return true;
        }

        const PostProcessSnapshot& ap = a.postProcess;
        const PostProcessSnapshot& bp = b.postProcess;
        if (ap.componentId != bp.componentId || ap.enabledComponent != bp.enabledComponent || ap.enabled != bp.enabled || ap.effect != bp.effect
            || ap.grayscaleEnabled != bp.grayscaleEnabled || ap.vignetteEnabled != bp.vignetteEnabled
            || ap.toneMappingEnabled != bp.toneMappingEnabled || ap.colorAdjustEnabled != bp.colorAdjustEnabled
            || Different(ap.grayscaleIntensity, bp.grayscaleIntensity, epsilon) || Different(ap.exposure, bp.exposure, epsilon)
            || Different(ap.gamma, bp.gamma, epsilon) || Different(ap.vignetteStrength, bp.vignetteStrength, epsilon)
            || Different(ap.vignetteRadius, bp.vignetteRadius, epsilon) || Different(ap.vignetteSoftness, bp.vignetteSoftness, epsilon)
            || Different(ap.brightness, bp.brightness, epsilon) || Different(ap.contrast, bp.contrast, epsilon)
            || Different(ap.saturation, bp.saturation, epsilon))
        {
            return true;
        }

        const PlayerStartSnapshot& aps = a.playerStart;
        const PlayerStartSnapshot& bps = b.playerStart;
        return aps.componentId != bps.componentId
            || aps.enabled != bps.enabled
            || Different(aps.playerHeight, bps.playerHeight, epsilon)
            || Different(aps.moveSpeed, bps.moveSpeed, epsilon)
            || Different(aps.fastMoveMultiplier, bps.fastMoveMultiplier, epsilon)
            || Different(aps.mouseSensitivity, bps.mouseSensitivity, epsilon);
    }

    void GameObjectSnapshot::ClearPersistentIDs()
    {
        id = Scene::InvalidObjectID;
        parentId = Scene::InvalidObjectID;
        children.clear();
        meshRenderer.componentId = Scene::InvalidComponentID;
        for (ColliderSnapshot& collider : colliders)
        {
            collider.componentId = Scene::InvalidComponentID;
        }
        light.componentId = Scene::InvalidComponentID;
        postProcess.componentId = Scene::InvalidComponentID;
        playerStart.componentId = Scene::InvalidComponentID;
    }

    void GameObjectSnapshot::ApplyTo(Scene::GameObject& object) const
    {
        if (id != Scene::InvalidObjectID)
        {
            object.SetID(id);
        }
        object.SetParentID(parentId);
        object.ClearChildren();
        for (Scene::ObjectID child : children)
        {
            object.AddChildID(child);
        }
        object.SetName(name);
        object.SetOutlinerFolder(outlinerFolder);
        object.GetTransform() = transform;

        object.RemoveComponents<Scene::MeshRendererComponent>();
        if (meshRenderer.enabled)
        {
            Scene::MeshRendererComponent& component = object.AddComponent<Scene::MeshRendererComponent>();
            if (meshRenderer.componentId != Scene::InvalidComponentID)
            {
                component.SetID(meshRenderer.componentId);
            }
            component.SetMesh(meshRenderer.mesh);
            component.SetMaterial(meshRenderer.cloneMaterialOnRestore && meshRenderer.material
                ? CloneMaterialInstance(*meshRenderer.material)
                : meshRenderer.material);
            component.SetMeshPath(meshRenderer.meshPath);
            component.SetMaterialPath(meshRenderer.cloneMaterialOnRestore ? std::wstring() : meshRenderer.materialPath);
            component.SetMeshGuid(meshRenderer.meshGuid);
            component.SetMaterialGuid(meshRenderer.cloneMaterialOnRestore ? std::string() : meshRenderer.materialGuid);
            ApplyMaterialState(component, meshRenderer);
        }

        object.RemoveComponents<Physics::ColliderComponent>();
        for (const ColliderSnapshot& colliderSnapshot : colliders)
        {
            Physics::ColliderComponent& collider = object.AddComponent<Physics::ColliderComponent>();
            if (colliderSnapshot.componentId != Scene::InvalidComponentID)
            {
                collider.SetID(colliderSnapshot.componentId);
            }
            collider.type = colliderSnapshot.type;
            collider.center = colliderSnapshot.center;
            collider.rotation = colliderSnapshot.rotation;
            collider.size = colliderSnapshot.size;
            collider.radius = colliderSnapshot.radius;
            collider.isTrigger = colliderSnapshot.isTrigger;
        }

        object.RemoveComponents<Scene::LightComponent>();
        if (light.enabled)
        {
            Scene::LightComponent& component = object.AddComponent<Scene::LightComponent>();
            if (light.componentId != Scene::InvalidComponentID)
            {
                component.SetID(light.componentId);
            }
            component.type = light.type;
            component.color = light.color;
            component.intensity = light.intensity;
            component.range = light.range;
            component.direction = light.direction;
            component.innerConeAngle = light.innerConeAngle;
            component.outerConeAngle = light.outerConeAngle;
            component.ambientColor = light.ambientColor;
        }

        object.RemoveComponents<Scene::PostProcessComponent>();
        if (postProcess.enabledComponent)
        {
            Scene::PostProcessComponent& component = object.AddComponent<Scene::PostProcessComponent>();
            if (postProcess.componentId != Scene::InvalidComponentID)
            {
                component.SetID(postProcess.componentId);
            }
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

        object.RemoveComponents<Scene::PlayerStartComponent>();
        if (playerStart.enabled)
        {
            Scene::PlayerStartComponent& component = object.AddComponent<Scene::PlayerStartComponent>();
            if (playerStart.componentId != Scene::InvalidComponentID)
            {
                component.SetID(playerStart.componentId);
            }
            component.playerHeight = playerStart.playerHeight;
            component.moveSpeed = playerStart.moveSpeed;
            component.fastMoveMultiplier = playerStart.fastMoveMultiplier;
            component.mouseSensitivity = playerStart.mouseSensitivity;
        }
    }

    Scene::GameObject& GameObjectSnapshot::Restore(Scene::Scene& scene) const
    {
        Scene::GameObject& object = id == Scene::InvalidObjectID
            ? scene.CreateGameObject(name)
            : scene.CreateGameObject(name, id);
        ApplyTo(object);
        return object;
    }
}
