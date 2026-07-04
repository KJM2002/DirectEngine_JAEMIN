#pragma once

#include "Engine/Graphics/Renderer/Mesh.h"

namespace Engine::Editor
{
    class PreviewCamera
    {
    public:
        void Reset();
        void FrameBounds(const Renderer::BoundingBox& bounds);
        void UpdateInput(float deltaTime, bool viewportHovered);
        void SetViewportSize(float width, float height);

        float GetYaw() const;
        float GetPitch() const;
        float GetDistance() const;
        const Math::Vector3& GetTarget() const;

    private:
        Math::Vector3 m_target = { 0.0f, 0.0f, 0.0f };
        float m_distance = 5.0f;
        float m_yaw = 0.0f;
        float m_pitch = 20.0f;
        float m_viewportWidth = 512.0f;
        float m_viewportHeight = 512.0f;
    };
}
