#pragma once

#include "Engine/Editor/MeshEditor/PreviewCamera.h"
#include "Engine/Physics/ColliderComponent.h"
#include "imgui.h"

#include <array>
#include <memory>
#include <vector>

namespace Engine::Renderer
{
    class Mesh;
    class Renderer;
}

namespace Engine::Editor
{
    enum class ColliderPreviewMode
    {
        None,
        BoundsBox,
        CustomCollider
    };

    enum class ColliderGizmoMode
    {
        Move,
        Rotate,
        Scale
    };

    class StaticMeshPreviewViewport
    {
    public:
        enum class GizmoAxis
        {
            None,
            X,
            Y,
            Z
        };

        struct ColliderPreview
        {
            Physics::ColliderType type = Physics::ColliderType::AABB;
            Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 size = { 1.0f, 1.0f, 1.0f };
            float radius = 0.5f;
            bool isTrigger = false;
        };

        void SetMesh(std::shared_ptr<Renderer::Mesh> mesh);
        void SetColliders(const std::vector<ColliderPreview>& colliders);
        void SetShowGrid(bool value);
        void SetShowAxis(bool value);
        void SetShowBounds(bool value);
        void SetShowCollider(bool value);
        void SetSelectedColliderIndex(int index);
        void SetGizmoMode(ColliderGizmoMode mode);
        void Update(float deltaTime);
        bool DrawImGuiViewport(Renderer::Renderer& renderer, std::vector<ColliderPreview>& colliders, int& selectedColliderIndex);
        void ResetCamera();
        void FrameMesh();

    private:
        ImVec2 ProjectPoint(const Math::Vector3& point, const ImVec2& origin, const ImVec2& size) const;
        std::array<Math::Vector3, 3> GetColliderAxes(const ColliderPreview& collider) const;
        void DrawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawAxis(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawMeshWireframe(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawBounds(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawColliderPreview(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        bool DrawColliderGizmo(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size, bool viewportHovered);
        int PickColliderIndex(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size) const;
        GizmoAxis PickGizmoAxis(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size) const;
        void ApplyGizmoDrag(const ImVec2& origin, const ImVec2& size);

        std::shared_ptr<Renderer::Mesh> m_mesh;
        std::vector<ColliderPreview> m_colliders;
        PreviewCamera m_camera;
        bool m_showGrid = true;
        bool m_showAxis = true;
        bool m_showBounds = false;
        bool m_showCollider = false;
        int m_selectedColliderIndex = -1;
        ColliderGizmoMode m_gizmoMode = ColliderGizmoMode::Move;
        GizmoAxis m_activeGizmoAxis = GizmoAxis::None;
        bool m_gizmoDragging = false;
        ImVec2 m_dragStartMouse = {};
        ColliderPreview m_dragStartCollider;
        float m_width = 512.0f;
        float m_height = 512.0f;
    };
}
