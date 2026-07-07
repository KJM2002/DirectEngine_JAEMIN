#include "GizmoRenderer.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIBuffer.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Scene/Camera.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Engine::Renderer
{
    namespace
    {
        Math::Vector3 Add(const Math::Vector3& a, const Math::Vector3& b)
        {
            return { a.x + b.x, a.y + b.y, a.z + b.z };
        }

        Math::Vector3 Subtract(const Math::Vector3& a, const Math::Vector3& b)
        {
            return { a.x - b.x, a.y - b.y, a.z - b.z };
        }

        Math::Vector3 Scale(const Math::Vector3& value, float amount)
        {
            return { value.x * amount, value.y * amount, value.z * amount };
        }

        float Length(const Math::Vector3& value)
        {
            return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
        }

        Math::Vector3 Normalize(const Math::Vector3& value, const Math::Vector3& fallback = { 0.0f, 1.0f, 0.0f })
        {
            const float length = Length(value);
            if (length <= 0.00001f || !std::isfinite(length))
            {
                return fallback;
            }

            return { value.x / length, value.y / length, value.z / length };
        }

        Math::Vector3 Cross(const Math::Vector3& a, const Math::Vector3& b)
        {
            return
            {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }

        Math::Vector3 RotateAxis(const Math::Vector3& rotationValue, const Math::Vector3& axis)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(rotationValue.x, rotationValue.y, rotationValue.z);
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&axis), rotation)));
            return result;
        }

        Math::Vector3 BuildLineSide(const Math::Vector3& viewDirection, const Math::Vector3& lineDirection)
        {
            Math::Vector3 side = Cross(viewDirection, lineDirection);
            if (Length(side) <= 0.00001f)
            {
                const Math::Vector3 reference = std::fabs(lineDirection.y) < 0.92f
                    ? Math::Vector3{ 0.0f, 1.0f, 0.0f }
                    : Math::Vector3{ 1.0f, 0.0f, 0.0f };
                side = Cross(reference, lineDirection);
            }

            if (Length(side) <= 0.00001f)
            {
                side = Cross({ 0.0f, 0.0f, 1.0f }, lineDirection);
            }

            return Normalize(side, { 1.0f, 0.0f, 0.0f });
        }

        Math::Vector4 AxisColor(GizmoAxis axis, GizmoAxis hoveredAxis, GizmoAxis activeAxis)
        {
            if (axis == activeAxis)
            {
                return { 1.0f, 0.82f, 0.12f, 1.0f };
            }
            if (axis == hoveredAxis)
            {
                return { 1.0f, 1.0f, 0.35f, 1.0f };
            }

            switch (axis)
            {
            case GizmoAxis::X:
                return { 1.0f, 0.16f, 0.14f, 1.0f };
            case GizmoAxis::Y:
                return { 0.22f, 0.95f, 0.22f, 1.0f };
            case GizmoAxis::Z:
                return { 0.24f, 0.48f, 1.0f, 1.0f };
            case GizmoAxis::None:
            default:
                return { 1.0f, 1.0f, 1.0f, 1.0f };
            }
        }

        float ComputeGizmoScale(const GizmoRenderDesc& desc)
        {
            if (!desc.camera)
            {
                return 1.0f;
            }

            const float distance = std::max(0.1f, Length(Subtract(desc.position, desc.camera->position)));
            return std::clamp(distance * 0.18f, 0.35f, 20.0f);
        }
    }

    bool GizmoRenderer::Initialize(RHI::RHIDevice& device)
    {
        m_device = &device;
        m_vertexShader = device.CreateVertexShader(L"Shaders/GizmoVertex.hlsl", "main");
        m_pixelShader = device.CreatePixelShader(L"Shaders/GizmoPixel.hlsl", "main");
        m_transformBuffer = device.CreateConstantBuffer(sizeof(GizmoTransformBuffer));

        if (!m_vertexShader || !m_pixelShader || !m_transformBuffer)
        {
            Core::Log::Error("Failed to initialize GizmoRenderer.");
            return false;
        }

        return true;
    }

    void GizmoRenderer::Shutdown()
    {
        m_vertices.clear();
        m_transformBuffer.reset();
        m_pixelShader.reset();
        m_vertexShader.reset();
        m_device = nullptr;
        m_hoveredAxis = GizmoAxis::None;
        m_activeAxis = GizmoAxis::None;
    }

    void GizmoRenderer::BeginFrame()
    {
        m_vertices.clear();
    }

    void GizmoRenderer::RenderMoveGizmo(const GizmoRenderDesc& desc)
    {
        if (!desc.camera)
        {
            return;
        }

        SetHoveredAxis(desc.hoveredAxis);
        SetActiveAxis(desc.activeAxis);

        const float scale = ComputeGizmoScale(desc);
        const float axisLength = scale * 1.25f;
        const float thickness = std::clamp(scale * 0.04f, 0.018f, 0.42f);
        const float arrowLength = scale * 0.24f;
        const float arrowWidth = scale * 0.15f;
        const Math::Vector3 axes[] =
        {
            RotateAxis(desc.rotation, { 1.0f, 0.0f, 0.0f }),
            RotateAxis(desc.rotation, { 0.0f, 1.0f, 0.0f }),
            RotateAxis(desc.rotation, { 0.0f, 0.0f, 1.0f })
        };
        const GizmoAxis axisIds[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

        AddBillboardQuad(desc.position, scale * 0.12f, { 1.0f, 0.95f, 0.25f, 1.0f }, *desc.camera);

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            const Math::Vector4 color = AxisColor(axisIds[axisIndex], m_hoveredAxis, m_activeAxis);
            const Math::Vector3 axis = axes[axisIndex];
            const Math::Vector3 end = Add(desc.position, Scale(axis, axisLength));
            const Math::Vector3 arrowBase = Add(desc.position, Scale(axis, axisLength - arrowLength));
            const Math::Vector3 sideA = Scale(axes[(axisIndex + 1) % 3], arrowWidth);
            const Math::Vector3 sideB = Scale(axes[(axisIndex + 2) % 3], arrowWidth);

            AddThickLine(desc.position, end, thickness, color, *desc.camera);
            AddThickLine(end, Add(arrowBase, sideA), thickness * 0.72f, color, *desc.camera);
            AddThickLine(end, Add(arrowBase, Scale(sideA, -1.0f)), thickness * 0.72f, color, *desc.camera);
            AddThickLine(end, Add(arrowBase, sideB), thickness * 0.72f, color, *desc.camera);
            AddThickLine(end, Add(arrowBase, Scale(sideB, -1.0f)), thickness * 0.72f, color, *desc.camera);
        }
    }

    void GizmoRenderer::RenderRotateGizmo(const GizmoRenderDesc& desc)
    {
        (void)desc;
    }

    void GizmoRenderer::RenderScaleGizmo(const GizmoRenderDesc& desc)
    {
        (void)desc;
    }

    void GizmoRenderer::EndFrame(RHI::RHIContext& context, const GizmoRenderDesc& desc)
    {
        if (!m_device || !desc.camera || m_vertices.empty())
        {
            m_vertices.clear();
            return;
        }

        std::shared_ptr<RHI::RHIBuffer> vertexBuffer = m_device->CreateVertexBuffer(
            m_vertices.data(),
            static_cast<std::uint32_t>(m_vertices.size() * sizeof(GizmoVertex)),
            sizeof(GizmoVertex));
        if (!vertexBuffer)
        {
            m_vertices.clear();
            return;
        }

        const float aspectRatio = desc.viewportHeight <= 0.0f ? 1.0f : desc.viewportWidth / desc.viewportHeight;
        GizmoTransformBuffer transform = {};
        DirectX::XMStoreFloat4x4(&transform.world, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&transform.view, desc.camera->GetViewMatrix());
        DirectX::XMStoreFloat4x4(&transform.projection, desc.camera->GetProjectionMatrix(aspectRatio));

        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 2, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 3, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 4, nullptr);
        context.UpdateConstantBuffer(m_transformBuffer, &transform, sizeof(transform));
        context.SetConstantBuffer(RHI::ShaderStage::Vertex, 0, m_transformBuffer);
        context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
        context.SetVertexBuffer(vertexBuffer);
        context.SetVertexShader(m_vertexShader);
        context.SetPixelShader(m_pixelShader);
        context.Draw(static_cast<std::uint32_t>(m_vertices.size()), 0);
        m_vertices.clear();
    }

    void GizmoRenderer::SetHoveredAxis(GizmoAxis axis)
    {
        m_hoveredAxis = axis;
    }

    void GizmoRenderer::SetActiveAxis(GizmoAxis axis)
    {
        m_activeAxis = axis;
    }

    void GizmoRenderer::AddThickLine(const Math::Vector3& start, const Math::Vector3& end, float thickness, const Math::Vector4& color, const Scene::Camera& camera)
    {
        const Math::Vector3 lineDirection = Normalize(Subtract(end, start), { 1.0f, 0.0f, 0.0f });
        const Math::Vector3 midpoint = Scale(Add(start, end), 0.5f);
        const Math::Vector3 viewDirection = Normalize(Subtract(camera.position, midpoint), { 0.0f, 0.0f, 1.0f });
        const Math::Vector3 side = BuildLineSide(viewDirection, lineDirection);

        const Math::Vector3 offset = Scale(side, thickness * 0.5f);
        const Math::Vector3 a = Add(start, offset);
        const Math::Vector3 b = Add(end, offset);
        const Math::Vector3 c = Add(end, Scale(offset, -1.0f));
        const Math::Vector3 d = Add(start, Scale(offset, -1.0f));

        AddVertex(a, color);
        AddVertex(b, color);
        AddVertex(c, color);
        AddVertex(a, color);
        AddVertex(c, color);
        AddVertex(d, color);
        AddVertex(c, color);
        AddVertex(b, color);
        AddVertex(a, color);
        AddVertex(d, color);
        AddVertex(c, color);
        AddVertex(a, color);
    }

    void GizmoRenderer::AddBillboardQuad(const Math::Vector3& center, float size, const Math::Vector4& color, const Scene::Camera& camera)
    {
        const float cosPitch = std::cos(camera.pitch);
        const Math::Vector3 forward = Normalize({
            std::sin(camera.yaw) * cosPitch,
            -std::sin(camera.pitch),
            std::cos(camera.yaw) * cosPitch
        }, { 0.0f, 0.0f, 1.0f });
        const Math::Vector3 right = Normalize(Cross({ 0.0f, 1.0f, 0.0f }, forward), { 1.0f, 0.0f, 0.0f });
        const Math::Vector3 up = Normalize(Cross(forward, right), { 0.0f, 1.0f, 0.0f });
        const Math::Vector3 halfRight = Scale(right, size * 0.5f);
        const Math::Vector3 halfUp = Scale(up, size * 0.5f);

        const Math::Vector3 a = Add(Add(center, halfRight), halfUp);
        const Math::Vector3 b = Add(Add(center, Scale(halfRight, -1.0f)), halfUp);
        const Math::Vector3 c = Add(Add(center, Scale(halfRight, -1.0f)), Scale(halfUp, -1.0f));
        const Math::Vector3 d = Add(Add(center, halfRight), Scale(halfUp, -1.0f));

        AddVertex(a, color);
        AddVertex(b, color);
        AddVertex(c, color);
        AddVertex(a, color);
        AddVertex(c, color);
        AddVertex(d, color);
        AddVertex(c, color);
        AddVertex(b, color);
        AddVertex(a, color);
        AddVertex(d, color);
        AddVertex(c, color);
        AddVertex(a, color);
    }

    void GizmoRenderer::AddVertex(const Math::Vector3& position, const Math::Vector4& color)
    {
        GizmoVertex vertex = {};
        vertex.position[0] = position.x;
        vertex.position[1] = position.y;
        vertex.position[2] = position.z;
        vertex.color[0] = color.x;
        vertex.color[1] = color.y;
        vertex.color[2] = color.z;
        vertex.color[3] = color.w;
        m_vertices.push_back(vertex);
    }
}
