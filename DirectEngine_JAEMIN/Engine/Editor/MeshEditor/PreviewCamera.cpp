#include "PreviewCamera.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace Engine::Editor
{
    void PreviewCamera::Reset()
    {
        m_target = { 0.0f, 0.0f, 0.0f };
        m_distance = 5.0f;
        m_yaw = 0.0f;
        m_pitch = 20.0f;
    }

    void PreviewCamera::FrameBounds(const Renderer::BoundingBox& bounds)
    {
        if (!bounds.IsValid())
        {
            Reset();
            return;
        }

        const Math::Vector3 extents = bounds.GetExtents();
        const float radius = std::max(0.1f, std::sqrt(extents.x * extents.x + extents.y * extents.y + extents.z * extents.z));
        m_target = bounds.GetCenter();
        m_distance = radius * 2.4f;
    }

    void PreviewCamera::UpdateInput(float, bool viewportHovered)
    {
        if (!viewportHovered)
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            m_yaw += io.MouseDelta.x * 0.35f;
            m_pitch += io.MouseDelta.y * 0.35f;
            m_pitch = std::clamp(m_pitch, -85.0f, 85.0f);
        }

        if (io.MouseWheel != 0.0f)
        {
            m_distance *= (io.MouseWheel > 0.0f) ? 0.88f : 1.12f;
            m_distance = std::clamp(m_distance, 0.1f, 1000.0f);
        }
    }

    void PreviewCamera::SetViewportSize(float width, float height)
    {
        m_viewportWidth = std::max(1.0f, width);
        m_viewportHeight = std::max(1.0f, height);
    }

    float PreviewCamera::GetYaw() const { return m_yaw; }
    float PreviewCamera::GetPitch() const { return m_pitch; }
    float PreviewCamera::GetDistance() const { return m_distance; }
    const Math::Vector3& PreviewCamera::GetTarget() const { return m_target; }

    Math::Matrix PreviewCamera::GetViewMatrix() const
    {
        const float yaw = m_yaw * 0.017453292f;
        const float pitch = m_pitch * 0.017453292f;
        const float cp = std::cos(pitch);
        const Math::Vector3 offset =
        {
            std::sin(yaw) * cp * m_distance,
            std::sin(pitch) * m_distance,
            std::cos(yaw) * cp * m_distance
        };
        const Math::Vector3 position =
        {
            m_target.x + offset.x,
            m_target.y + offset.y,
            m_target.z + offset.z
        };

        return DirectX::XMMatrixLookAtLH(
            DirectX::XMLoadFloat3(&position),
            DirectX::XMLoadFloat3(&m_target),
            DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    }

    Math::Matrix PreviewCamera::GetProjectionMatrix() const
    {
        const float aspect = m_viewportHeight <= 0.0f ? 1.0f : m_viewportWidth / m_viewportHeight;
        return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), aspect, 0.01f, 5000.0f);
    }
}
