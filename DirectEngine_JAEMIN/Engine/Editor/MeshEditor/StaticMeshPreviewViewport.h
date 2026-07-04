#pragma once

#include "Engine/Editor/MeshEditor/PreviewCamera.h"
#include "Engine/Physics/ColliderComponent.h"
#include "imgui.h"

#include <memory>

namespace Engine::Renderer
{
    class Mesh;
}

namespace Engine::Editor
{
    enum class ColliderPreviewMode
    {
        None,
        BoundsBox,
        CustomCollider
    };

    class StaticMeshPreviewViewport
    {
    public:
        struct ColliderPreview
        {
            Physics::ColliderType type = Physics::ColliderType::AABB;
            Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
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
        void Update(float deltaTime);
        void DrawImGuiViewport();
        void ResetCamera();
        void FrameMesh();

    private:
        ImVec2 ProjectPoint(const Math::Vector3& point, const ImVec2& origin, const ImVec2& size) const;
        void DrawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawAxis(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawMeshWireframe(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawBounds(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;
        void DrawColliderPreview(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) const;

        std::shared_ptr<Renderer::Mesh> m_mesh;
        std::vector<ColliderPreview> m_colliders;
        PreviewCamera m_camera;
        bool m_showGrid = true;
        bool m_showAxis = true;
        bool m_showBounds = true;
        bool m_showCollider = false;
        float m_width = 512.0f;
        float m_height = 512.0f;
    };
}
