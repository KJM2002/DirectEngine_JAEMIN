#include "EditorCamera.h"

#include "Engine/Input/Input.h"
#include "Engine/Scene/Camera.h"

#include <algorithm>
#include <cmath>

namespace Engine::Editor
{
    namespace
    {
        constexpr float MaxPitch = DirectX::XM_PIDIV2 - 0.01f;

        Math::Vector3 StoreVector3(DirectX::FXMVECTOR vector)
        {
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, vector);
            return result;
        }

        Math::Vector3 GetForward(float yaw, float pitch)
        {
            const float cosPitch = std::cos(pitch);
            return StoreVector3(DirectX::XMVector3Normalize(DirectX::XMVectorSet(
                std::sin(yaw) * cosPitch,
                -std::sin(pitch),
                std::cos(yaw) * cosPitch,
                0.0f)));
        }
    }

    void EditorCamera::Update(const Input::Input& input, float deltaTime)
    {
        const bool navigating = input.GetMouseButtonRaw(Input::MouseButtonRight);
        if (navigating)
        {
            yaw += static_cast<float>(input.GetMouseDeltaX()) * mouseSensitivity;
            pitch += static_cast<float>(input.GetMouseDeltaY()) * mouseSensitivity;
            pitch = std::clamp(pitch, -MaxPitch, MaxPitch);
        }

        if (!navigating)
        {
            return;
        }

        const float speed = input.GetKeyRaw(Input::KeyShift) ? moveSpeed * fastMoveMultiplier : moveSpeed;
        const float distance = speed * deltaTime;
        const Math::Vector3 forwardVector = GetForward(yaw, pitch);
        const DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&forwardVector);
        const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, forward));
        DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat3(&position);

        if (input.GetKeyRaw(Input::KeyW))
        {
            positionVector = DirectX::XMVectorAdd(positionVector, DirectX::XMVectorScale(forward, distance));
        }
        if (input.GetKeyRaw(Input::KeyS))
        {
            positionVector = DirectX::XMVectorSubtract(positionVector, DirectX::XMVectorScale(forward, distance));
        }
        if (input.GetKeyRaw(Input::KeyD))
        {
            positionVector = DirectX::XMVectorAdd(positionVector, DirectX::XMVectorScale(right, distance));
        }
        if (input.GetKeyRaw(Input::KeyA))
        {
            positionVector = DirectX::XMVectorSubtract(positionVector, DirectX::XMVectorScale(right, distance));
        }
        if (input.GetKeyRaw(Input::KeyE))
        {
            positionVector = DirectX::XMVectorAdd(positionVector, DirectX::XMVectorScale(up, distance));
        }
        if (input.GetKeyRaw(Input::KeyQ))
        {
            positionVector = DirectX::XMVectorSubtract(positionVector, DirectX::XMVectorScale(up, distance));
        }

        DirectX::XMStoreFloat3(&position, positionVector);
    }

    void EditorCamera::ApplyTo(Scene::Camera& camera) const
    {
        camera.position = position;
        camera.yaw = yaw;
        camera.pitch = pitch;
        camera.fov = fov;
        camera.nearPlane = nearPlane;
        camera.farPlane = farPlane;
    }

    void EditorCamera::SetFrom(const Scene::Camera& camera)
    {
        position = camera.position;
        yaw = camera.yaw;
        pitch = camera.pitch;
        fov = camera.fov;
        nearPlane = camera.nearPlane;
        farPlane = camera.farPlane;
    }
}
