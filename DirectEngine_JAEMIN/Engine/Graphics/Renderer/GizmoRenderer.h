#pragma once

#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Math/MathTypes.h"

#include <memory>
#include <vector>

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIDevice;
    class RHIContext;
    class RHIShader;
}

namespace Engine::Scene
{
    class Camera;
}

namespace Engine::Renderer
{
    struct GizmoRenderDesc
    {
        const Scene::Camera* camera = nullptr;
        Math::Vector3 position = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };
        float viewportWidth = 1.0f;
        float viewportHeight = 1.0f;
        GizmoVisualMode mode = GizmoVisualMode::Move;
        GizmoAxis hoveredAxis = GizmoAxis::None;
        GizmoAxis activeAxis = GizmoAxis::None;
    };

    class GizmoRenderer
    {
    public:
        bool Initialize(RHI::RHIDevice& device);
        void Shutdown();

        void BeginFrame();
        void RenderMoveGizmo(const GizmoRenderDesc& desc);
        void RenderRotateGizmo(const GizmoRenderDesc& desc);
        void RenderScaleGizmo(const GizmoRenderDesc& desc);
        void EndFrame(RHI::RHIContext& context, const GizmoRenderDesc& desc);

        void SetHoveredAxis(GizmoAxis axis);
        void SetActiveAxis(GizmoAxis axis);

    private:
        struct GizmoVertex
        {
            float position[3] = {};
            float normal[3] = {};
            float color[4] = {};
            float uv[2] = {};
            float tangent[3] = {};
        };

        struct GizmoTransformBuffer
        {
            Math::Matrix4x4 world;
            Math::Matrix4x4 view;
            Math::Matrix4x4 projection;
        };

        void AddThickLine(const Math::Vector3& start, const Math::Vector3& end, float thickness, const Math::Vector4& color, const Scene::Camera& camera);
        void AddBillboardQuad(const Math::Vector3& center, float size, const Math::Vector4& color, const Scene::Camera& camera);
        void AddVertex(const Math::Vector3& position, const Math::Vector4& color);

        RHI::RHIDevice* m_device = nullptr;
        std::shared_ptr<RHI::RHIShader> m_vertexShader;
        std::shared_ptr<RHI::RHIShader> m_pixelShader;
        std::shared_ptr<RHI::RHIBuffer> m_transformBuffer;
        std::vector<GizmoVertex> m_vertices;
        GizmoAxis m_hoveredAxis = GizmoAxis::None;
        GizmoAxis m_activeAxis = GizmoAxis::None;
    };
}
