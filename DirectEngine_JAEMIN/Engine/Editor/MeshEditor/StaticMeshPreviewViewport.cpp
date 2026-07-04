#include "StaticMeshPreviewViewport.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Engine::Editor
{
    void StaticMeshPreviewViewport::SetMesh(std::shared_ptr<Renderer::Mesh> mesh)
    {
        m_mesh = std::move(mesh);
        FrameMesh();
    }

    void StaticMeshPreviewViewport::SetColliders(const std::vector<ColliderPreview>& colliders)
    {
        m_colliders = colliders;
    }

    void StaticMeshPreviewViewport::SetShowGrid(bool value) { m_showGrid = value; }
    void StaticMeshPreviewViewport::SetShowAxis(bool value) { m_showAxis = value; }
    void StaticMeshPreviewViewport::SetShowBounds(bool value) { m_showBounds = value; }
    void StaticMeshPreviewViewport::SetShowCollider(bool value) { m_showCollider = value; }

    void StaticMeshPreviewViewport::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    void StaticMeshPreviewViewport::DrawImGuiViewport()
    {
        ImVec2 available = ImGui::GetContentRegionAvail();
        available.x = std::max(160.0f, available.x);
        available.y = std::max(160.0f, available.y);
        m_width = available.x;
        m_height = available.y;
        m_camera.SetViewportSize(m_width, m_height);

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("StaticMeshPreviewViewport", available, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        const bool hovered = ImGui::IsItemHovered();
        m_camera.UpdateInput(0.0f, hovered);
        if (hovered && ImGui::IsKeyPressed(ImGuiKey_F))
        {
            FrameMesh();
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 max = ImVec2(origin.x + available.x, origin.y + available.y);
        drawList->AddRectFilled(origin, max, IM_COL32(18, 19, 23, 255));
        drawList->AddRect(origin, max, IM_COL32(75, 80, 90, 255));

        if (m_showGrid)
        {
            DrawGrid(drawList, origin, available);
        }
        if (m_showAxis)
        {
            DrawAxis(drawList, origin, available);
        }
        DrawMeshWireframe(drawList, origin, available);
        if (m_showBounds)
        {
            DrawBounds(drawList, origin, available);
        }
        if (m_showCollider && m_mesh && m_mesh->GetBounds().IsValid())
        {
            DrawColliderPreview(drawList, origin, available);
            drawList->AddText(ImVec2(origin.x + 12.0f, origin.y + available.y - 24.0f), IM_COL32(245, 210, 80, 255), "Collider preview: BoundsBox");
        }
        if (!m_mesh)
        {
            const char* text = "No Static Mesh selected.";
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            drawList->AddText(ImVec2(origin.x + (available.x - textSize.x) * 0.5f, origin.y + (available.y - textSize.y) * 0.5f), IM_COL32(210, 215, 225, 255), text);
        }
        else
        {
            drawList->AddText(ImVec2(origin.x + 12.0f, origin.y + 12.0f), IM_COL32(210, 215, 225, 255), "Preview canvas - offscreen mesh render target hook prepared");
        }
    }

    void StaticMeshPreviewViewport::ResetCamera()
    {
        m_camera.Reset();
    }

    void StaticMeshPreviewViewport::FrameMesh()
    {
        if (m_mesh)
        {
            m_camera.FrameBounds(m_mesh->GetBounds());
        }
        else
        {
            m_camera.Reset();
        }
    }

    ImVec2 StaticMeshPreviewViewport::ProjectPoint(const Math::Vector3& point, const ImVec2& origin, const ImVec2& size) const
    {
        const float yaw = m_camera.GetYaw() * 0.017453292f;
        const float pitch = m_camera.GetPitch() * 0.017453292f;
        const float cy = std::cos(yaw);
        const float sy = std::sin(yaw);
        const float cp = std::cos(pitch);
        const float sp = std::sin(pitch);
        const Math::Vector3 target = m_camera.GetTarget();
        const float x = point.x - target.x;
        const float y = point.y - target.y;
        const float z = point.z - target.z;
        const float rx = x * cy - z * sy;
        const float rz = x * sy + z * cy;
        const float ry = y * cp - rz * sp;
        const float scale = std::min(size.x, size.y) / std::max(0.1f, m_camera.GetDistance() * 2.0f);
        return ImVec2(origin.x + size.x * 0.5f + rx * scale, origin.y + size.y * 0.5f - ry * scale);
    }

    void StaticMeshPreviewViewport::DrawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        constexpr int divisions = 20;
        constexpr float gridSize = 10.0f;
        for (int i = 0; i <= divisions; ++i)
        {
            const float t = -gridSize * 0.5f + gridSize * static_cast<float>(i) / static_cast<float>(divisions);
            const ImU32 color = (i == divisions / 2) ? IM_COL32(90, 95, 105, 170) : IM_COL32(58, 62, 70, 130);
            drawList->AddLine(ProjectPoint({ -gridSize * 0.5f, 0.0f, t }, origin, size), ProjectPoint({ gridSize * 0.5f, 0.0f, t }, origin, size), color);
            drawList->AddLine(ProjectPoint({ t, 0.0f, -gridSize * 0.5f }, origin, size), ProjectPoint({ t, 0.0f, gridSize * 0.5f }, origin, size), color);
        }
    }

    void StaticMeshPreviewViewport::DrawAxis(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 2.0f, 0.0f, 0.0f }, origin, size), IM_COL32(235, 70, 60, 255), 2.0f);
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 0.0f, 2.0f, 0.0f }, origin, size), IM_COL32(70, 210, 90, 255), 2.0f);
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 0.0f, 0.0f, 2.0f }, origin, size), IM_COL32(75, 145, 255, 255), 2.0f);
    }

    void StaticMeshPreviewViewport::DrawMeshWireframe(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        if (!m_mesh)
        {
            return;
        }

        const std::vector<Renderer::Vertex>& vertices = m_mesh->GetVertices();
        const std::vector<std::uint32_t>& indices = m_mesh->GetIndices();
        if (vertices.empty() || indices.size() < 3)
        {
            return;
        }

        const std::size_t maxIndexCount = std::min<std::size_t>(indices.size(), 12000);
        for (std::size_t i = 0; i + 2 < maxIndexCount; i += 3)
        {
            const std::uint32_t ia = indices[i];
            const std::uint32_t ib = indices[i + 1];
            const std::uint32_t ic = indices[i + 2];
            if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size())
            {
                continue;
            }

            const Math::Vector3 a = { vertices[ia].position[0], vertices[ia].position[1], vertices[ia].position[2] };
            const Math::Vector3 b = { vertices[ib].position[0], vertices[ib].position[1], vertices[ib].position[2] };
            const Math::Vector3 c = { vertices[ic].position[0], vertices[ic].position[1], vertices[ic].position[2] };
            const ImVec2 pa = ProjectPoint(a, origin, size);
            const ImVec2 pb = ProjectPoint(b, origin, size);
            const ImVec2 pc = ProjectPoint(c, origin, size);
            const ImU32 color = IM_COL32(175, 190, 210, 170);
            drawList->AddLine(pa, pb, color);
            drawList->AddLine(pb, pc, color);
            drawList->AddLine(pc, pa, color);
        }
    }

    void StaticMeshPreviewViewport::DrawBounds(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        if (!m_mesh || !m_mesh->GetBounds().IsValid())
        {
            return;
        }

        const Renderer::BoundingBox& bounds = m_mesh->GetBounds();
        const std::array<Math::Vector3, 8> points =
        {
            Math::Vector3{ bounds.min.x, bounds.min.y, bounds.min.z },
            Math::Vector3{ bounds.max.x, bounds.min.y, bounds.min.z },
            Math::Vector3{ bounds.max.x, bounds.max.y, bounds.min.z },
            Math::Vector3{ bounds.min.x, bounds.max.y, bounds.min.z },
            Math::Vector3{ bounds.min.x, bounds.min.y, bounds.max.z },
            Math::Vector3{ bounds.max.x, bounds.min.y, bounds.max.z },
            Math::Vector3{ bounds.max.x, bounds.max.y, bounds.max.z },
            Math::Vector3{ bounds.min.x, bounds.max.y, bounds.max.z }
        };
        const int edges[][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        for (const auto& edge : edges)
        {
            drawList->AddLine(ProjectPoint(points[edge[0]], origin, size), ProjectPoint(points[edge[1]], origin, size), IM_COL32(255, 205, 65, 255), 2.0f);
        }
    }

    void StaticMeshPreviewViewport::DrawColliderPreview(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        if (m_colliders.empty())
        {
            DrawBounds(drawList, origin, size);
            return;
        }

        for (const ColliderPreview& collider : m_colliders)
        {
            if (collider.type == Physics::ColliderType::Sphere)
            {
                constexpr int segments = 48;
                for (int i = 0; i < segments; ++i)
                {
                    const float a0 = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
                    const float a1 = DirectX::XM_2PI * static_cast<float>(i + 1) / static_cast<float>(segments);
                    const Math::Vector3 p0 = { collider.center.x + std::cos(a0) * collider.radius, collider.center.y, collider.center.z + std::sin(a0) * collider.radius };
                    const Math::Vector3 p1 = { collider.center.x + std::cos(a1) * collider.radius, collider.center.y, collider.center.z + std::sin(a1) * collider.radius };
                    drawList->AddLine(ProjectPoint(p0, origin, size), ProjectPoint(p1, origin, size), IM_COL32(80, 230, 180, 255), 2.0f);
                }
                continue;
            }

            const Math::Vector3 half = { collider.size.x * 0.5f, collider.size.y * 0.5f, collider.size.z * 0.5f };
            const Renderer::BoundingBox box =
            {
                { collider.center.x - half.x, collider.center.y - half.y, collider.center.z - half.z },
                { collider.center.x + half.x, collider.center.y + half.y, collider.center.z + half.z },
                true
            };
            const std::array<Math::Vector3, 8> points =
            {
                Math::Vector3{ box.min.x, box.min.y, box.min.z },
                Math::Vector3{ box.max.x, box.min.y, box.min.z },
                Math::Vector3{ box.max.x, box.max.y, box.min.z },
                Math::Vector3{ box.min.x, box.max.y, box.min.z },
                Math::Vector3{ box.min.x, box.min.y, box.max.z },
                Math::Vector3{ box.max.x, box.min.y, box.max.z },
                Math::Vector3{ box.max.x, box.max.y, box.max.z },
                Math::Vector3{ box.min.x, box.max.y, box.max.z }
            };
            const int edges[][2] =
            {
                {0, 1}, {1, 2}, {2, 3}, {3, 0},
                {4, 5}, {5, 6}, {6, 7}, {7, 4},
                {0, 4}, {1, 5}, {2, 6}, {3, 7}
            };
            for (const auto& edge : edges)
            {
                drawList->AddLine(ProjectPoint(points[edge[0]], origin, size), ProjectPoint(points[edge[1]], origin, size), IM_COL32(80, 230, 180, 255), 2.0f);
            }
        }
    }
}
