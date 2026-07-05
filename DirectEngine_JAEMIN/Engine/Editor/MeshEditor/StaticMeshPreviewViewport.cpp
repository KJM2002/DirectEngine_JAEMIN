#include "StaticMeshPreviewViewport.h"

#include "Engine/Graphics/Renderer/Renderer.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace Engine::Editor
{
    namespace
    {
        float Distance(const ImVec2& a, const ImVec2& b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        float DistancePointToSegment(const ImVec2& point, const ImVec2& a, const ImVec2& b)
        {
            const float vx = b.x - a.x;
            const float vy = b.y - a.y;
            const float wx = point.x - a.x;
            const float wy = point.y - a.y;
            const float lengthSq = vx * vx + vy * vy;
            if (lengthSq <= 0.0001f)
            {
                return Distance(point, a);
            }

            const float t = std::clamp((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);
            return Distance(point, ImVec2(a.x + vx * t, a.y + vy * t));
        }

        float DotScreen(const ImVec2& a, const ImVec2& b)
        {
            return a.x * b.x + a.y * b.y;
        }

        ImU32 AxisColor(StaticMeshPreviewViewport::GizmoAxis axis, bool active)
        {
            const int alpha = active ? 255 : 225;
            switch (axis)
            {
            case StaticMeshPreviewViewport::GizmoAxis::X:
                return IM_COL32(235, 70, 60, alpha);
            case StaticMeshPreviewViewport::GizmoAxis::Y:
                return IM_COL32(70, 210, 90, alpha);
            case StaticMeshPreviewViewport::GizmoAxis::Z:
                return IM_COL32(75, 145, 255, alpha);
            case StaticMeshPreviewViewport::GizmoAxis::None:
            default:
                return IM_COL32(245, 225, 65, alpha);
            }
        }

        int AxisIndex(StaticMeshPreviewViewport::GizmoAxis axis)
        {
            switch (axis)
            {
            case StaticMeshPreviewViewport::GizmoAxis::X:
                return 0;
            case StaticMeshPreviewViewport::GizmoAxis::Y:
                return 1;
            case StaticMeshPreviewViewport::GizmoAxis::Z:
                return 2;
            case StaticMeshPreviewViewport::GizmoAxis::None:
            default:
                return -1;
            }
        }
    }

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
    void StaticMeshPreviewViewport::SetSelectedColliderIndex(int index) { m_selectedColliderIndex = index; }
    void StaticMeshPreviewViewport::SetGizmoMode(ColliderGizmoMode mode) { m_gizmoMode = mode; }

    void StaticMeshPreviewViewport::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    bool StaticMeshPreviewViewport::DrawImGuiViewport(Renderer::Renderer& renderer, std::vector<ColliderPreview>& colliders, int& selectedColliderIndex)
    {
        m_colliders = colliders;
        m_selectedColliderIndex = selectedColliderIndex;
        bool changed = false;
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
        void* previewTexture = nullptr;
        if (m_mesh)
        {
            Math::Matrix4x4 view = {};
            Math::Matrix4x4 projection = {};
            DirectX::XMStoreFloat4x4(&view, m_camera.GetViewMatrix());
            DirectX::XMStoreFloat4x4(&projection, m_camera.GetProjectionMatrix());
            previewTexture = renderer.RenderStaticMeshPreview(*m_mesh, static_cast<std::uint32_t>(m_width), static_cast<std::uint32_t>(m_height), view, projection);
        }

        if (previewTexture)
        {
            drawList->AddImage(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(previewTexture)), origin, max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }
        else
        {
            drawList->AddRectFilled(origin, max, IM_COL32(18, 19, 23, 255));
        }
        drawList->AddRect(origin, max, IM_COL32(75, 80, 90, 255));

        if (m_showGrid)
        {
            DrawGrid(drawList, origin, available);
        }
        if (m_showAxis)
        {
            DrawAxis(drawList, origin, available);
        }
        if (!previewTexture)
        {
            DrawMeshWireframe(drawList, origin, available);
        }
        if (m_showBounds)
        {
            DrawBounds(drawList, origin, available);
        }
        if (m_showCollider && m_mesh && m_mesh->GetBounds().IsValid())
        {
            DrawColliderPreview(drawList, origin, available);
            changed = DrawColliderGizmo(drawList, origin, available, hovered) || changed;
            if (m_selectedColliderIndex != selectedColliderIndex)
            {
                selectedColliderIndex = m_selectedColliderIndex;
            }
            const std::string label = "Colliders: " + std::to_string(m_colliders.size());
            drawList->AddRectFilled(ImVec2(origin.x + 8.0f, origin.y + available.y - 30.0f), ImVec2(origin.x + 116.0f, origin.y + available.y - 8.0f), IM_COL32(10, 12, 16, 185), 4.0f);
            drawList->AddText(ImVec2(origin.x + 14.0f, origin.y + available.y - 26.0f), IM_COL32(245, 210, 80, 255), label.c_str());
        }
        if (!m_mesh)
        {
            const char* text = "No Static Mesh selected.";
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            drawList->AddText(ImVec2(origin.x + (available.x - textSize.x) * 0.5f, origin.y + (available.y - textSize.y) * 0.5f), IM_COL32(210, 215, 225, 255), text);
        }
        else
        {
            const char* label = previewTexture ? "RT Preview" : "Preview unavailable";
            drawList->AddRectFilled(ImVec2(origin.x + 8.0f, origin.y + 8.0f), ImVec2(origin.x + 142.0f, origin.y + 30.0f), IM_COL32(10, 12, 16, 185), 4.0f);
            drawList->AddText(ImVec2(origin.x + 14.0f, origin.y + 12.0f), IM_COL32(210, 215, 225, 255), label);
        }

        if (changed)
        {
            colliders = m_colliders;
        }
        return changed;
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
        const DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
        const DirectX::XMMATRIX view = m_camera.GetViewMatrix();
        const DirectX::XMMATRIX projection = m_camera.GetProjectionMatrix();
        const DirectX::XMVECTOR projected = DirectX::XMVector3Project(
            DirectX::XMLoadFloat3(&point),
            origin.x,
            origin.y,
            size.x,
            size.y,
            0.0f,
            1.0f,
            projection,
            view,
            world);

        Math::Vector3 result;
        DirectX::XMStoreFloat3(&result, projected);
        return ImVec2(result.x, result.y);
    }

    std::array<Math::Vector3, 3> StaticMeshPreviewViewport::GetColliderAxes(const ColliderPreview& collider) const
    {
        const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(collider.rotation.x, collider.rotation.y, collider.rotation.z);
        std::array<Math::Vector3, 3> axes = {};
        DirectX::XMStoreFloat3(&axes[0], DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation)));
        DirectX::XMStoreFloat3(&axes[1], DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation)));
        DirectX::XMStoreFloat3(&axes[2], DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation)));
        return axes;
    }

    void StaticMeshPreviewViewport::DrawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        constexpr int divisions = 20;
        constexpr float gridSize = 10.0f;
        for (int i = 0; i <= divisions; ++i)
        {
            const float t = -gridSize * 0.5f + gridSize * static_cast<float>(i) / static_cast<float>(divisions);
            const ImU32 color = (i == divisions / 2) ? IM_COL32(105, 110, 120, 95) : IM_COL32(72, 76, 86, 55);
            drawList->AddLine(ProjectPoint({ -gridSize * 0.5f, 0.0f, t }, origin, size), ProjectPoint({ gridSize * 0.5f, 0.0f, t }, origin, size), color);
            drawList->AddLine(ProjectPoint({ t, 0.0f, -gridSize * 0.5f }, origin, size), ProjectPoint({ t, 0.0f, gridSize * 0.5f }, origin, size), color);
        }
    }

    void StaticMeshPreviewViewport::DrawAxis(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 2.0f, 0.0f, 0.0f }, origin, size), IM_COL32(235, 70, 60, 210), 2.0f);
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 0.0f, 2.0f, 0.0f }, origin, size), IM_COL32(70, 210, 90, 210), 2.0f);
        drawList->AddLine(ProjectPoint({ 0.0f, 0.0f, 0.0f }, origin, size), ProjectPoint({ 0.0f, 0.0f, 2.0f }, origin, size), IM_COL32(75, 145, 255, 210), 2.0f);
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
            drawList->AddLine(ProjectPoint(points[edge[0]], origin, size), ProjectPoint(points[edge[1]], origin, size), IM_COL32(255, 205, 65, 205), 1.5f);
        }
    }

    void StaticMeshPreviewViewport::DrawColliderPreview(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const
    {
        if (m_colliders.empty())
        {
            DrawBounds(drawList, origin, size);
            return;
        }

        for (std::size_t colliderIndex = 0; colliderIndex < m_colliders.size(); ++colliderIndex)
        {
            const ColliderPreview& collider = m_colliders[colliderIndex];
            const bool selected = static_cast<int>(colliderIndex) == m_selectedColliderIndex;
            const ImU32 colliderColor = selected ? IM_COL32(255, 225, 65, 255) : IM_COL32(60, 235, 195, 230);
            const float thickness = selected ? 3.0f : 2.0f;

            if (collider.type == Physics::ColliderType::Sphere)
            {
                constexpr int segments = 48;
                const Math::Vector3 axisPairs[3][2] =
                {
                    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
                    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
                    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
                };

                for (const auto& axes : axisPairs)
                {
                    for (int i = 0; i < segments; ++i)
                    {
                        const float a0 = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
                        const float a1 = DirectX::XM_2PI * static_cast<float>(i + 1) / static_cast<float>(segments);
                        const Math::Vector3 p0 =
                        {
                            collider.center.x + (axes[0].x * std::cos(a0) + axes[1].x * std::sin(a0)) * collider.radius,
                            collider.center.y + (axes[0].y * std::cos(a0) + axes[1].y * std::sin(a0)) * collider.radius,
                            collider.center.z + (axes[0].z * std::cos(a0) + axes[1].z * std::sin(a0)) * collider.radius
                        };
                        const Math::Vector3 p1 =
                        {
                            collider.center.x + (axes[0].x * std::cos(a1) + axes[1].x * std::sin(a1)) * collider.radius,
                            collider.center.y + (axes[0].y * std::cos(a1) + axes[1].y * std::sin(a1)) * collider.radius,
                            collider.center.z + (axes[0].z * std::cos(a1) + axes[1].z * std::sin(a1)) * collider.radius
                        };
                        drawList->AddLine(ProjectPoint(p0, origin, size), ProjectPoint(p1, origin, size), colliderColor, thickness);
                    }
                }
                continue;
            }

            const Math::Vector3 half = { collider.size.x * 0.5f, collider.size.y * 0.5f, collider.size.z * 0.5f };
            const std::array<Math::Vector3, 3> axes = GetColliderAxes(collider);
            const int signs[8][3] =
            {
                {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
            };

            std::array<Math::Vector3, 8> points = {};
            for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
            {
                points[pointIndex] =
                {
                    collider.center.x + axes[0].x * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].x * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].x * half.z * static_cast<float>(signs[pointIndex][2]),
                    collider.center.y + axes[0].y * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].y * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].y * half.z * static_cast<float>(signs[pointIndex][2]),
                    collider.center.z + axes[0].z * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].z * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].z * half.z * static_cast<float>(signs[pointIndex][2])
                };
            }
            const int edges[][2] =
            {
                {0, 1}, {1, 2}, {2, 3}, {3, 0},
                {4, 5}, {5, 6}, {6, 7}, {7, 4},
                {0, 4}, {1, 5}, {2, 6}, {3, 7}
            };
            for (const auto& edge : edges)
            {
                drawList->AddLine(ProjectPoint(points[edge[0]], origin, size), ProjectPoint(points[edge[1]], origin, size), colliderColor, thickness);
            }
        }
    }

    bool StaticMeshPreviewViewport::DrawColliderGizmo(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size, bool viewportHovered)
    {
        if (m_selectedColliderIndex < 0 || static_cast<std::size_t>(m_selectedColliderIndex) >= m_colliders.size())
        {
            m_gizmoDragging = false;
            m_activeGizmoAxis = GizmoAxis::None;
            if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const int pickedCollider = PickColliderIndex(ImGui::GetIO().MousePos, origin, size);
                if (pickedCollider >= 0)
                {
                    m_selectedColliderIndex = pickedCollider;
                }
            }
            return false;
        }

        ColliderPreview& collider = m_colliders[static_cast<std::size_t>(m_selectedColliderIndex)];
        const std::array<Math::Vector3, 3> axes = GetColliderAxes(collider);
        const ImVec2 center = ProjectPoint(collider.center, origin, size);
        const float gizmoLength = std::max(0.35f, m_camera.GetDistance() * 0.18f);
        const GizmoAxis axisValues[3] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

        if (m_gizmoMode == ColliderGizmoMode::Rotate)
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                const Math::Vector3& axisA = axes[(axis + 1) % 3];
                const Math::Vector3& axisB = axes[(axis + 2) % 3];
                ImVec2 previous = {};
                constexpr int segments = 64;
                for (int i = 0; i <= segments; ++i)
                {
                    const float angle = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
                    const Math::Vector3 point =
                    {
                        collider.center.x + (axisA.x * std::cos(angle) + axisB.x * std::sin(angle)) * gizmoLength,
                        collider.center.y + (axisA.y * std::cos(angle) + axisB.y * std::sin(angle)) * gizmoLength,
                        collider.center.z + (axisA.z * std::cos(angle) + axisB.z * std::sin(angle)) * gizmoLength
                    };
                    const ImVec2 projected = ProjectPoint(point, origin, size);
                    if (i > 0)
                    {
                        drawList->AddLine(previous, projected, AxisColor(axisValues[axis], m_activeGizmoAxis == axisValues[axis]), m_activeGizmoAxis == axisValues[axis] ? 3.0f : 2.0f);
                    }
                    previous = projected;
                }
            }
        }
        else
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                const Math::Vector3 end =
                {
                    collider.center.x + axes[axis].x * gizmoLength,
                    collider.center.y + axes[axis].y * gizmoLength,
                    collider.center.z + axes[axis].z * gizmoLength
                };
                const ImVec2 projectedEnd = ProjectPoint(end, origin, size);
                const bool active = m_activeGizmoAxis == axisValues[axis];
                drawList->AddLine(center, projectedEnd, AxisColor(axisValues[axis], active), active ? 4.0f : 3.0f);

                if (m_gizmoMode == ColliderGizmoMode::Move)
                {
                    const ImVec2 direction(projectedEnd.x - center.x, projectedEnd.y - center.y);
                    const float length = std::max(1.0f, std::sqrt(direction.x * direction.x + direction.y * direction.y));
                    const ImVec2 unit(direction.x / length, direction.y / length);
                    const ImVec2 side(-unit.y, unit.x);
                    const ImVec2 base(projectedEnd.x - unit.x * 11.0f, projectedEnd.y - unit.y * 11.0f);
                    drawList->AddTriangleFilled(projectedEnd, ImVec2(base.x + side.x * 5.0f, base.y + side.y * 5.0f), ImVec2(base.x - side.x * 5.0f, base.y - side.y * 5.0f), AxisColor(axisValues[axis], active));
                }
                else
                {
                    drawList->AddRectFilled(ImVec2(projectedEnd.x - 5.0f, projectedEnd.y - 5.0f), ImVec2(projectedEnd.x + 5.0f, projectedEnd.y + 5.0f), AxisColor(axisValues[axis], active));
                }
            }
        }

        ImGuiIO& io = ImGui::GetIO();
        bool changed = false;
        if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_activeGizmoAxis = PickGizmoAxis(io.MousePos, origin, size);
            if (m_activeGizmoAxis != GizmoAxis::None)
            {
                m_gizmoDragging = true;
                m_dragStartMouse = io.MousePos;
                m_dragStartCollider = collider;
            }
            else
            {
                const int pickedCollider = PickColliderIndex(io.MousePos, origin, size);
                if (pickedCollider >= 0)
                {
                    m_selectedColliderIndex = pickedCollider;
                }
            }
        }

        if (m_gizmoDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            ApplyGizmoDrag(origin, size);
            changed = true;
        }
        else if (m_gizmoDragging)
        {
            m_gizmoDragging = false;
            m_activeGizmoAxis = GizmoAxis::None;
        }

        return changed;
    }

    int StaticMeshPreviewViewport::PickColliderIndex(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size) const
    {
        if (m_colliders.empty())
        {
            return -1;
        }

        int bestIndex = -1;
        float bestDistance = 16.0f;
        constexpr int boxEdges[][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        constexpr int sphereSegments = 48;
        const Math::Vector3 sphereAxisPairs[3][2] =
        {
            { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
            { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        for (std::size_t colliderIndex = 0; colliderIndex < m_colliders.size(); ++colliderIndex)
        {
            const ColliderPreview& collider = m_colliders[colliderIndex];
            if (collider.type == Physics::ColliderType::Sphere)
            {
                for (const auto& axes : sphereAxisPairs)
                {
                    ImVec2 previous = {};
                    for (int i = 0; i <= sphereSegments; ++i)
                    {
                        const float angle = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(sphereSegments);
                        const Math::Vector3 point =
                        {
                            collider.center.x + (axes[0].x * std::cos(angle) + axes[1].x * std::sin(angle)) * collider.radius,
                            collider.center.y + (axes[0].y * std::cos(angle) + axes[1].y * std::sin(angle)) * collider.radius,
                            collider.center.z + (axes[0].z * std::cos(angle) + axes[1].z * std::sin(angle)) * collider.radius
                        };
                        const ImVec2 projected = ProjectPoint(point, origin, size);
                        if (i > 0)
                        {
                            const float distance = DistancePointToSegment(mouse, previous, projected);
                            if (distance < bestDistance)
                            {
                                bestDistance = distance;
                                bestIndex = static_cast<int>(colliderIndex);
                            }
                        }
                        previous = projected;
                    }
                }
                continue;
            }

            const Math::Vector3 half = { collider.size.x * 0.5f, collider.size.y * 0.5f, collider.size.z * 0.5f };
            const std::array<Math::Vector3, 3> axes = GetColliderAxes(collider);
            const int signs[8][3] =
            {
                {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
            };

            std::array<ImVec2, 8> projectedPoints = {};
            for (std::size_t pointIndex = 0; pointIndex < projectedPoints.size(); ++pointIndex)
            {
                const Math::Vector3 point =
                {
                    collider.center.x + axes[0].x * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].x * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].x * half.z * static_cast<float>(signs[pointIndex][2]),
                    collider.center.y + axes[0].y * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].y * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].y * half.z * static_cast<float>(signs[pointIndex][2]),
                    collider.center.z + axes[0].z * half.x * static_cast<float>(signs[pointIndex][0]) + axes[1].z * half.y * static_cast<float>(signs[pointIndex][1]) + axes[2].z * half.z * static_cast<float>(signs[pointIndex][2])
                };
                projectedPoints[pointIndex] = ProjectPoint(point, origin, size);
            }

            for (const auto& edge : boxEdges)
            {
                const float distance = DistancePointToSegment(mouse, projectedPoints[edge[0]], projectedPoints[edge[1]]);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestIndex = static_cast<int>(colliderIndex);
                }
            }
        }

        return bestIndex;
    }

    StaticMeshPreviewViewport::GizmoAxis StaticMeshPreviewViewport::PickGizmoAxis(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size) const
    {
        if (m_selectedColliderIndex < 0 || static_cast<std::size_t>(m_selectedColliderIndex) >= m_colliders.size())
        {
            return GizmoAxis::None;
        }

        const ColliderPreview& collider = m_colliders[static_cast<std::size_t>(m_selectedColliderIndex)];
        const std::array<Math::Vector3, 3> axes = GetColliderAxes(collider);
        const float gizmoLength = std::max(0.35f, m_camera.GetDistance() * 0.18f);
        const GizmoAxis axisValues[3] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };
        float bestDistance = 14.0f;
        GizmoAxis bestAxis = GizmoAxis::None;
        const ImVec2 center = ProjectPoint(collider.center, origin, size);

        if (m_gizmoMode == ColliderGizmoMode::Rotate)
        {
            constexpr int segments = 64;
            for (int axis = 0; axis < 3; ++axis)
            {
                const Math::Vector3& axisA = axes[(axis + 1) % 3];
                const Math::Vector3& axisB = axes[(axis + 2) % 3];
                ImVec2 previous = {};
                for (int i = 0; i <= segments; ++i)
                {
                    const float angle = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
                    const Math::Vector3 point =
                    {
                        collider.center.x + (axisA.x * std::cos(angle) + axisB.x * std::sin(angle)) * gizmoLength,
                        collider.center.y + (axisA.y * std::cos(angle) + axisB.y * std::sin(angle)) * gizmoLength,
                        collider.center.z + (axisA.z * std::cos(angle) + axisB.z * std::sin(angle)) * gizmoLength
                    };
                    const ImVec2 projected = ProjectPoint(point, origin, size);
                    if (i > 0)
                    {
                        const float distance = DistancePointToSegment(mouse, previous, projected);
                        if (distance < bestDistance)
                        {
                            bestDistance = distance;
                            bestAxis = axisValues[axis];
                        }
                    }
                    previous = projected;
                }
            }
            return bestAxis;
        }

        for (int axis = 0; axis < 3; ++axis)
        {
            const Math::Vector3 end =
            {
                collider.center.x + axes[axis].x * gizmoLength,
                collider.center.y + axes[axis].y * gizmoLength,
                collider.center.z + axes[axis].z * gizmoLength
            };
            const float distance = DistancePointToSegment(mouse, center, ProjectPoint(end, origin, size));
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestAxis = axisValues[axis];
            }
        }
        return bestAxis;
    }

    void StaticMeshPreviewViewport::ApplyGizmoDrag(const ImVec2& origin, const ImVec2& size)
    {
        if (m_selectedColliderIndex < 0 || static_cast<std::size_t>(m_selectedColliderIndex) >= m_colliders.size())
        {
            return;
        }

        const int axisIndex = AxisIndex(m_activeGizmoAxis);
        if (axisIndex < 0)
        {
            return;
        }

        ColliderPreview& collider = m_colliders[static_cast<std::size_t>(m_selectedColliderIndex)];
        const std::array<Math::Vector3, 3> axes = GetColliderAxes(m_dragStartCollider);
        const float gizmoLength = std::max(0.35f, m_camera.GetDistance() * 0.18f);
        const ImVec2 center = ProjectPoint(m_dragStartCollider.center, origin, size);
        const Math::Vector3 end =
        {
            m_dragStartCollider.center.x + axes[axisIndex].x * gizmoLength,
            m_dragStartCollider.center.y + axes[axisIndex].y * gizmoLength,
            m_dragStartCollider.center.z + axes[axisIndex].z * gizmoLength
        };
        ImVec2 screenAxis(ProjectPoint(end, origin, size).x - center.x, ProjectPoint(end, origin, size).y - center.y);
        const float axisLength = std::sqrt(screenAxis.x * screenAxis.x + screenAxis.y * screenAxis.y);
        if (axisLength <= 0.001f)
        {
            return;
        }
        screenAxis.x /= axisLength;
        screenAxis.y /= axisLength;

        ImGuiIO& io = ImGui::GetIO();
        const ImVec2 mouseDelta(io.MousePos.x - m_dragStartMouse.x, io.MousePos.y - m_dragStartMouse.y);
        const float projectedDelta = DotScreen(mouseDelta, screenAxis);
        const float worldDelta = projectedDelta * std::max(0.001f, m_camera.GetDistance()) / std::max(1.0f, std::min(size.x, size.y));

        if (m_gizmoMode == ColliderGizmoMode::Move)
        {
            collider.center =
            {
                m_dragStartCollider.center.x + axes[axisIndex].x * worldDelta,
                m_dragStartCollider.center.y + axes[axisIndex].y * worldDelta,
                m_dragStartCollider.center.z + axes[axisIndex].z * worldDelta
            };
        }
        else if (m_gizmoMode == ColliderGizmoMode::Scale)
        {
            if (collider.type == Physics::ColliderType::Sphere)
            {
                collider.radius = std::max(0.01f, m_dragStartCollider.radius + worldDelta);
            }
            else
            {
                collider.size = m_dragStartCollider.size;
                float* components[3] = { &collider.size.x, &collider.size.y, &collider.size.z };
                *components[axisIndex] = std::max(0.01f, *components[axisIndex] + worldDelta * 2.0f);
            }
        }
        else if (m_gizmoMode == ColliderGizmoMode::Rotate)
        {
            collider.rotation = m_dragStartCollider.rotation;
            float* components[3] = { &collider.rotation.x, &collider.rotation.y, &collider.rotation.z };
            *components[axisIndex] += projectedDelta * 0.01f;
        }
    }
}
