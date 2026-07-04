#include "Camera.h"

#include "Engine/Input/Input.h"

#include <algorithm>
#include <cmath>

namespace Engine::Scene
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
    }

    void Camera::UpdateFromInput(const Input::Input& input, float deltaTime)
    {
        if (input.IsMouseCaptured())
        {
            yaw += static_cast<float>(input.GetMouseDeltaX()) * mouseSensitivity;
            pitch += static_cast<float>(input.GetMouseDeltaY()) * mouseSensitivity;
            pitch = std::clamp(pitch, -MaxPitch, MaxPitch);
        }

        const float speed = input.GetKey(Input::KeyShift) ? moveSpeed * fastMoveMultiplier : moveSpeed;
        const float distance = speed * deltaTime;

        const Math::Vector3 forwardVector = GetForward();
        const Math::Vector3 rightVector = GetRight();
        const DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&forwardVector);
        const DirectX::XMVECTOR right = DirectX::XMLoadFloat3(&rightVector);
        DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat3(&position);

        if (input.GetKey(Input::KeyW))
        {
            positionVector = DirectX::XMVectorAdd(positionVector, DirectX::XMVectorScale(forward, distance));
        }
        if (input.GetKey(Input::KeyS))
        {
            positionVector = DirectX::XMVectorSubtract(positionVector, DirectX::XMVectorScale(forward, distance));
        }
        if (input.GetKey(Input::KeyD))
        {
            positionVector = DirectX::XMVectorAdd(positionVector, DirectX::XMVectorScale(right, distance));
        }
        if (input.GetKey(Input::KeyA))
        {
            positionVector = DirectX::XMVectorSubtract(positionVector, DirectX::XMVectorScale(right, distance));
        }

        DirectX::XMStoreFloat3(&position, positionVector);
    }

    Math::Matrix Camera::GetViewMatrix() const
    {
        const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&position);
        const Math::Vector3 forwardVector = GetForward();
        const DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&forwardVector);
        const DirectX::XMVECTOR focus = DirectX::XMVectorAdd(eye, forward);
        const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return DirectX::XMMatrixLookAtLH(eye, focus, up);
    }

    Math::Matrix Camera::GetProjectionMatrix(float aspectRatio) const
    {
        return DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
    }

    Math::Vector3 Camera::GetForward() const
    {
        const float cosPitch = std::cos(pitch);
        const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSet(
            std::sin(yaw) * cosPitch,
            -std::sin(pitch),
            std::cos(yaw) * cosPitch,
            0.0f));
        return StoreVector3(forward);
    }

    Math::Vector3 Camera::GetRight() const
    {
        const Math::Vector3 forwardVector = GetForward();
        const DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&forwardVector);
        const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return StoreVector3(DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, forward)));
    }
}
