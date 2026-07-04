#include "PhysicsWorld.h"

#include "ColliderComponent.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Engine::Physics
{
    namespace
    {
        DirectX::XMVECTOR Load(const Math::Vector3& value)
        {
            return DirectX::XMLoadFloat3(&value);
        }

        Math::Vector3 Store(DirectX::FXMVECTOR value)
        {
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, value);
            return result;
        }

        bool RaycastBox(const Ray& ray, const ColliderComponent& collider, float& outDistance)
        {
            const Math::Vector3 center = collider.GetWorldCenter();
            const Math::Vector3 size = collider.GetWorldSize();
            const Math::Vector3 half = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
            const Math::Vector3 axes[3] = { collider.GetWorldAxisX(), collider.GetWorldAxisY(), collider.GetWorldAxisZ() };
            const float halfExtents[3] = { half.x, half.y, half.z };

            const DirectX::XMVECTOR origin = Load(ray.origin);
            const DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(Load(ray.direction));
            const DirectX::XMVECTOR delta = DirectX::XMVectorSubtract(Load(center), origin);
            float tMin = 0.0f;
            float tMax = std::numeric_limits<float>::max();

            for (int axis = 0; axis < 3; ++axis)
            {
                const DirectX::XMVECTOR worldAxis = Load(axes[axis]);
                const float e = DirectX::XMVectorGetX(DirectX::XMVector3Dot(worldAxis, delta));
                const float f = DirectX::XMVectorGetX(DirectX::XMVector3Dot(direction, worldAxis));

                if (std::abs(f) < 0.00001f)
                {
                    if (-e - halfExtents[axis] > 0.0f || -e + halfExtents[axis] < 0.0f)
                    {
                        return false;
                    }
                    continue;
                }

                const float invF = 1.0f / f;
                float t0 = (e + halfExtents[axis]) * invF;
                float t1 = (e - halfExtents[axis]) * invF;
                if (t0 > t1)
                {
                    std::swap(t0, t1);
                }

                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);
                if (tMax < tMin)
                {
                    return false;
                }
            }

            outDistance = tMin;
            return true;
        }

        bool RaycastSphere(const Ray& ray, const ColliderComponent& collider, float& outDistance)
        {
            const DirectX::XMVECTOR origin = Load(ray.origin);
            const DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(Load(ray.direction));
            const DirectX::XMVECTOR center = Load(collider.GetWorldCenter());
            const DirectX::XMVECTOR oc = DirectX::XMVectorSubtract(origin, center);
            const float b = DirectX::XMVectorGetX(DirectX::XMVector3Dot(oc, direction));
            const float c = DirectX::XMVectorGetX(DirectX::XMVector3Dot(oc, oc)) - collider.GetWorldRadius() * collider.GetWorldRadius();
            const float discriminant = b * b - c;
            if (discriminant < 0.0f)
            {
                return false;
            }

            const float sqrtD = std::sqrt(discriminant);
            float t = -b - sqrtD;
            if (t < 0.0f)
            {
                t = -b + sqrtD;
            }
            if (t < 0.0f)
            {
                return false;
            }

            outDistance = t;
            return true;
        }

        Math::Vector3 Subtract(const Math::Vector3& a, const Math::Vector3& b)
        {
            return { a.x - b.x, a.y - b.y, a.z - b.z };
        }

        Math::Vector3 Add(const Math::Vector3& a, const Math::Vector3& b)
        {
            return { a.x + b.x, a.y + b.y, a.z + b.z };
        }

        Math::Vector3 Scale(const Math::Vector3& value, float amount)
        {
            return { value.x * amount, value.y * amount, value.z * amount };
        }

        float Dot(const Math::Vector3& a, const Math::Vector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        Math::Vector3 HalfSize(const ColliderComponent& collider)
        {
            const Math::Vector3 size = collider.GetWorldSize();
            return { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        }

        bool TestBoxAxis(const Math::Vector3& axis, const Math::Vector3& centerDelta, const Math::Vector3 firstAxes[3], const Math::Vector3 secondAxes[3], const Math::Vector3& firstHalf, const Math::Vector3& secondHalf)
        {
            const DirectX::XMVECTOR loadedAxis = Load(axis);
            if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(loadedAxis)) < 0.000001f)
            {
                return true;
            }

            const DirectX::XMVECTOR axisVector = DirectX::XMVector3Normalize(loadedAxis);
            const Math::Vector3 normalizedAxis = Store(axisVector);
            const float distance = std::abs(Dot(centerDelta, normalizedAxis));
            const float firstRadius =
                firstHalf.x * std::abs(Dot(firstAxes[0], normalizedAxis)) +
                firstHalf.y * std::abs(Dot(firstAxes[1], normalizedAxis)) +
                firstHalf.z * std::abs(Dot(firstAxes[2], normalizedAxis));
            const float secondRadius =
                secondHalf.x * std::abs(Dot(secondAxes[0], normalizedAxis)) +
                secondHalf.y * std::abs(Dot(secondAxes[1], normalizedAxis)) +
                secondHalf.z * std::abs(Dot(secondAxes[2], normalizedAxis));

            return distance <= firstRadius + secondRadius;
        }

        bool OverlapBox(const ColliderComponent& first, const ColliderComponent& second)
        {
            const Math::Vector3 firstAxes[3] = { first.GetWorldAxisX(), first.GetWorldAxisY(), first.GetWorldAxisZ() };
            const Math::Vector3 secondAxes[3] = { second.GetWorldAxisX(), second.GetWorldAxisY(), second.GetWorldAxisZ() };
            const Math::Vector3 firstHalf = HalfSize(first);
            const Math::Vector3 secondHalf = HalfSize(second);
            const Math::Vector3 centerDelta = Subtract(second.GetWorldCenter(), first.GetWorldCenter());

            for (int i = 0; i < 3; ++i)
            {
                if (!TestBoxAxis(firstAxes[i], centerDelta, firstAxes, secondAxes, firstHalf, secondHalf)
                    || !TestBoxAxis(secondAxes[i], centerDelta, firstAxes, secondAxes, firstHalf, secondHalf))
                {
                    return false;
                }
            }

            for (int firstAxis = 0; firstAxis < 3; ++firstAxis)
            {
                for (int secondAxis = 0; secondAxis < 3; ++secondAxis)
                {
                    Math::Vector3 crossAxis;
                    DirectX::XMStoreFloat3(&crossAxis, DirectX::XMVector3Cross(Load(firstAxes[firstAxis]), Load(secondAxes[secondAxis])));
                    if (!TestBoxAxis(crossAxis, centerDelta, firstAxes, secondAxes, firstHalf, secondHalf))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool OverlapSphere(const ColliderComponent& first, const ColliderComponent& second)
        {
            const Math::Vector3 delta = Subtract(first.GetWorldCenter(), second.GetWorldCenter());
            const float radius = first.GetWorldRadius() + second.GetWorldRadius();
            return Dot(delta, delta) <= radius * radius;
        }

        bool OverlapBoxSphere(const ColliderComponent& box, const ColliderComponent& sphere)
        {
            const Math::Vector3 boxCenter = box.GetWorldCenter();
            const Math::Vector3 sphereCenter = sphere.GetWorldCenter();
            const Math::Vector3 half = HalfSize(box);
            const Math::Vector3 axes[3] = { box.GetWorldAxisX(), box.GetWorldAxisY(), box.GetWorldAxisZ() };
            const Math::Vector3 deltaToSphere = Subtract(sphereCenter, boxCenter);

            Math::Vector3 closest = boxCenter;
            const float halfExtents[3] = { half.x, half.y, half.z };
            for (int axis = 0; axis < 3; ++axis)
            {
                const float projected = std::clamp(Dot(deltaToSphere, axes[axis]), -halfExtents[axis], halfExtents[axis]);
                closest = Add(closest, Scale(axes[axis], projected));
            }

            const Math::Vector3 sphereToClosest = Subtract(sphereCenter, closest);
            return Dot(sphereToClosest, sphereToClosest) <= sphere.GetWorldRadius() * sphere.GetWorldRadius();
        }
    }

    void PhysicsWorld::BuildFromScene(const Scene::Scene& scene)
    {
        m_colliders.clear();
        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            for (ColliderComponent* collider : object->GetComponents<ColliderComponent>())
            {
                if (collider)
                {
                    m_colliders.push_back(collider);
                }
            }
        }
    }

    bool PhysicsWorld::Raycast(const Ray& ray, RaycastHit& outHit) const
    {
        float closestDistance = std::numeric_limits<float>::max();
        ColliderComponent* closestCollider = nullptr;

        for (ColliderComponent* collider : m_colliders)
        {
            float distance = 0.0f;
            const bool hit = collider->type == ColliderType::AABB
                ? RaycastBox(ray, *collider, distance)
                : RaycastSphere(ray, *collider, distance);

            if (hit && distance < closestDistance)
            {
                closestDistance = distance;
                closestCollider = collider;
            }
        }

        if (!closestCollider)
        {
            return false;
        }

        const DirectX::XMVECTOR point = DirectX::XMVectorAdd(Load(ray.origin), DirectX::XMVectorScale(DirectX::XMVector3Normalize(Load(ray.direction)), closestDistance));
        outHit.object = closestCollider->GetOwner();
        outHit.point = Store(point);
        outHit.distance = closestDistance;
        return true;
    }

    bool PhysicsWorld::Overlap(const ColliderComponent& first, const ColliderComponent& second) const
    {
        if (first.type == ColliderType::AABB && second.type == ColliderType::AABB)
        {
            return OverlapBox(first, second);
        }
        if (first.type == ColliderType::Sphere && second.type == ColliderType::Sphere)
        {
            return OverlapSphere(first, second);
        }
        if (first.type == ColliderType::AABB)
        {
            return OverlapBoxSphere(first, second);
        }
        return OverlapBoxSphere(second, first);
    }

    std::vector<CollisionPair> PhysicsWorld::UpdateOverlaps() const
    {
        std::vector<CollisionPair> pairs;
        for (std::size_t i = 0; i < m_colliders.size(); ++i)
        {
            for (std::size_t j = i + 1; j < m_colliders.size(); ++j)
            {
                ColliderComponent* first = m_colliders[i];
                ColliderComponent* second = m_colliders[j];
                if (!first || !second || first->GetOwner() == second->GetOwner())
                {
                    continue;
                }

                if (Overlap(*first, *second))
                {
                    pairs.push_back({ first, second, first->isTrigger || second->isTrigger });
                }
            }
        }
        return pairs;
    }
}
