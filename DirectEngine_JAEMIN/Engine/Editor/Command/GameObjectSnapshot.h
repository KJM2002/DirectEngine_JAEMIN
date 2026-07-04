#pragma once

#include "Engine/Math/Transform.h"
#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Graphics/Renderer/Material.h"
#include "Engine/Graphics/Renderer/Mesh.h"
#include "Engine/Graphics/Renderer/PostProcess.h"
#include "Engine/Scene/LightComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/PostProcessComponent.h"

#include <memory>
#include <string>
#include <vector>

namespace Engine::Scene
{
    class GameObject;
    class Scene;
}

namespace Engine::Editor
{
    struct MeshRendererSnapshot
    {
        bool enabled = false;
        std::shared_ptr<Renderer::Mesh> mesh;
        std::shared_ptr<Renderer::Material> material;
        std::wstring meshPath;
        std::wstring materialPath;
        std::string meshGuid;
        std::string materialGuid;
        bool cloneMaterialOnRestore = false;
    };

    struct ColliderSnapshot
    {
        Physics::ColliderType type = Physics::ColliderType::AABB;
        Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 size = { 1.0f, 1.0f, 1.0f };
        float radius = 0.5f;
        bool isTrigger = false;
    };

    struct LightSnapshot
    {
        bool enabled = false;
        Scene::LightType type = Scene::LightType::Point;
        Math::Vector3 color = { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float range = 5.0f;
        Math::Vector3 direction = { 0.0f, -1.0f, 0.0f };
        float innerConeAngle = 20.0f;
        float outerConeAngle = 35.0f;
        Math::Vector3 ambientColor = { 0.0f, 0.0f, 0.0f };
    };

    struct PostProcessSnapshot
    {
        bool enabledComponent = false;
        bool enabled = true;
        Renderer::PostProcessEffect effect = Renderer::PostProcessEffect::None;
        bool grayscaleEnabled = false;
        bool vignetteEnabled = false;
        bool toneMappingEnabled = false;
        bool colorAdjustEnabled = false;
        float grayscaleIntensity = 1.0f;
        float exposure = 1.0f;
        float gamma = 2.2f;
        float vignetteStrength = 0.35f;
        float vignetteRadius = 0.85f;
        float vignetteSoftness = 0.45f;
        float brightness = 0.0f;
        float contrast = 1.0f;
        float saturation = 1.0f;
    };

    struct PlayerStartSnapshot
    {
        bool enabled = false;
        float playerHeight = 1.7f;
        float moveSpeed = 4.0f;
        float fastMoveMultiplier = 3.0f;
        float mouseSensitivity = 0.003f;
    };

    struct GameObjectSnapshot
    {
        std::string name = "GameObject";
        Math::Transform transform;
        MeshRendererSnapshot meshRenderer;
        std::vector<ColliderSnapshot> colliders;
        LightSnapshot light;
        PostProcessSnapshot postProcess;
        PlayerStartSnapshot playerStart;

        static GameObjectSnapshot Capture(const Scene::GameObject& object);
        Scene::GameObject& Restore(Scene::Scene& scene) const;
    };
}
