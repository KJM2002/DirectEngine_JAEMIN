#pragma once

#include "Engine/Math/MathTypes.h"
#include "Engine/Scene/Component.h"

namespace Engine::Physics
{
    enum class ColliderType
    {
        AABB,
        Sphere
    };

    class ColliderComponent : public Scene::Component
    {
    public:
        ColliderType type = ColliderType::AABB;
        Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 size = { 1.0f, 1.0f, 1.0f };
        float radius = 0.5f;
        bool isTrigger = false;

        Math::Vector3 GetWorldCenter() const;
        Math::Vector3 GetWorldSize() const;
        Math::Vector3 GetWorldAxisX() const;
        Math::Vector3 GetWorldAxisY() const;
        Math::Vector3 GetWorldAxisZ() const;
        float GetWorldRadius() const;
    };
}
