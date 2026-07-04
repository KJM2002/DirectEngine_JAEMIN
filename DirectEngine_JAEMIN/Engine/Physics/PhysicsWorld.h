#pragma once

#include "Ray.h"
#include "RaycastHit.h"

#include <vector>

namespace Engine::Physics
{
    class ColliderComponent;
}

namespace Engine::Scene
{
    class Scene;
}

namespace Engine::Physics
{
    struct CollisionPair
    {
        ColliderComponent* first = nullptr;
        ColliderComponent* second = nullptr;
        bool isTrigger = false;
    };

    class PhysicsWorld
    {
    public:
        void BuildFromScene(const Scene::Scene& scene);
        bool Raycast(const Ray& ray, RaycastHit& outHit) const;
        bool Overlap(const ColliderComponent& first, const ColliderComponent& second) const;
        std::vector<CollisionPair> UpdateOverlaps() const;

    private:
        std::vector<ColliderComponent*> m_colliders;
    };
}
