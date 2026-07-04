#pragma once

#include "Engine/Math/MathTypes.h"

#include <string>

namespace Engine::Input
{
    class Input;
}

namespace Engine::Scene
{
    class Camera
    {
    public:
        void UpdateFromInput(const Input::Input& input, float deltaTime);

        Math::Matrix GetViewMatrix() const;
        Math::Matrix GetProjectionMatrix(float aspectRatio) const;

        Math::Vector3 position = { 0.0f, 0.0f, -3.0f };
        std::string name = "MainCamera";
        bool active = true;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fov = DirectX::XM_PIDIV4;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        float moveSpeed = 3.0f;
        float fastMoveMultiplier = 3.0f;
        float mouseSensitivity = 0.003f;

    private:
        Math::Vector3 GetForward() const;
        Math::Vector3 GetRight() const;
    };
}
