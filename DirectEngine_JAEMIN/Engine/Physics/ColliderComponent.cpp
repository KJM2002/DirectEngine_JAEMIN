#include "ColliderComponent.h"

#include "Engine/Scene/GameObject.h"

#include <algorithm>

namespace Engine::Physics
{
    Math::Vector3 ColliderComponent::GetWorldCenter() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return center;
        }

        const Math::Transform& transform = owner->GetTransform();
        const DirectX::XMVECTOR localCenter = DirectX::XMVectorSet(center.x * transform.scale.x, center.y * transform.scale.y, center.z * transform.scale.z, 0.0f);
        const DirectX::XMMATRIX ownerRotation = DirectX::XMMatrixRotationRollPitchYaw(transform.rotation.x, transform.rotation.y, transform.rotation.z);
        const DirectX::XMVECTOR worldCenter = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&transform.position), DirectX::XMVector3TransformNormal(localCenter, ownerRotation));

        Math::Vector3 result;
        DirectX::XMStoreFloat3(&result, worldCenter);
        return result;
    }

    Math::Vector3 ColliderComponent::GetWorldSize() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return size;
        }

        const Math::Transform& transform = owner->GetTransform();
        return
        {
            std::abs(size.x * transform.scale.x),
            std::abs(size.y * transform.scale.y),
            std::abs(size.z * transform.scale.z)
        };
    }

    float ColliderComponent::GetWorldRadius() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return radius;
        }

        const Math::Vector3& scale = owner->GetTransform().scale;
        const float maxScale = std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) });
        return radius * maxScale;
    }

    Math::Vector3 ColliderComponent::GetWorldAxisX() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return { 1.0f, 0.0f, 0.0f };
        }

        const Math::Vector3& ownerRotationValue = owner->GetTransform().rotation;
        const DirectX::XMMATRIX colliderRotation = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        const DirectX::XMMATRIX ownerRotation = DirectX::XMMatrixRotationRollPitchYaw(ownerRotationValue.x, ownerRotationValue.y, ownerRotationValue.z);
        const DirectX::XMMATRIX rotationMatrix = colliderRotation * ownerRotation;
        Math::Vector3 result;
        DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix)));
        return result;
    }

    Math::Vector3 ColliderComponent::GetWorldAxisY() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return { 0.0f, 1.0f, 0.0f };
        }

        const Math::Vector3& ownerRotationValue = owner->GetTransform().rotation;
        const DirectX::XMMATRIX colliderRotation = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        const DirectX::XMMATRIX ownerRotation = DirectX::XMMatrixRotationRollPitchYaw(ownerRotationValue.x, ownerRotationValue.y, ownerRotationValue.z);
        const DirectX::XMMATRIX rotationMatrix = colliderRotation * ownerRotation;
        Math::Vector3 result;
        DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix)));
        return result;
    }

    Math::Vector3 ColliderComponent::GetWorldAxisZ() const
    {
        const Scene::GameObject* owner = GetOwner();
        if (!owner)
        {
            return { 0.0f, 0.0f, 1.0f };
        }

        const Math::Vector3& ownerRotationValue = owner->GetTransform().rotation;
        const DirectX::XMMATRIX colliderRotation = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        const DirectX::XMMATRIX ownerRotation = DirectX::XMMatrixRotationRollPitchYaw(ownerRotationValue.x, ownerRotationValue.y, ownerRotationValue.z);
        const DirectX::XMMATRIX rotationMatrix = colliderRotation * ownerRotation;
        Math::Vector3 result;
        DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix)));
        return result;
    }
}
