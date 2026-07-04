#pragma once

#include "Engine/Math/MathTypes.h"

namespace Engine::Input
{
    class Input;
}

namespace Engine::Scene
{
    class Camera;
}

namespace Engine::Editor
{
    class EditorCamera
    {
    public:
        void Update(const Input::Input& input, float deltaTime);
        void ApplyTo(Scene::Camera& camera) const;
        void SetFrom(const Scene::Camera& camera);

        Math::Vector3 position = { 0.0f, 1.5f, -4.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fov = DirectX::XMConvertToRadians(60.0f);
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        float moveSpeed = 4.0f;
        float fastMoveMultiplier = 3.0f;
        float mouseSensitivity = 0.003f;
    };
}
