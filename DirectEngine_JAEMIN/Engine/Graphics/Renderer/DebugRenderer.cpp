#include "DebugRenderer.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIBuffer.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"

#include <array>
#include <cmath>

namespace Engine::Renderer
{
    bool DebugRenderer::Initialize(RHI::RHIDevice& device)
    {
        m_device = &device;
        m_vertexShader = device.CreateVertexShader(L"Shaders/DebugLineVertex.hlsl", "main");
        m_pixelShader = device.CreatePixelShader(L"Shaders/DebugLinePixel.hlsl", "main");
        m_transformBuffer = device.CreateConstantBuffer(sizeof(DebugTransformBuffer));

        if (!m_vertexShader || !m_pixelShader || !m_transformBuffer)
        {
            Core::Log::Error("Failed to initialize DebugRenderer.");
            return false;
        }

        return true;
    }

    void DebugRenderer::Shutdown()
    {
        m_vertices.clear();
        m_transformBuffer.reset();
        m_pixelShader.reset();
        m_vertexShader.reset();
        m_device = nullptr;
    }

    void DebugRenderer::DrawLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color)
    {
        LineVertex a = {};
        LineVertex b = {};
        a.position[0] = start.x;
        a.position[1] = start.y;
        a.position[2] = start.z;
        b.position[0] = end.x;
        b.position[1] = end.y;
        b.position[2] = end.z;
        a.color[0] = b.color[0] = color.x;
        a.color[1] = b.color[1] = color.y;
        a.color[2] = b.color[2] = color.z;
        a.color[3] = b.color[3] = color.w;
        m_vertices.push_back(a);
        m_vertices.push_back(b);
    }

    void DebugRenderer::DrawAABB(const Math::Vector3& center, const Math::Vector3& size, const Math::Vector4& color)
    {
        const Math::Vector3 half = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        const std::array<Math::Vector3, 8> points =
        {
            Math::Vector3{ center.x - half.x, center.y - half.y, center.z - half.z },
            Math::Vector3{ center.x + half.x, center.y - half.y, center.z - half.z },
            Math::Vector3{ center.x + half.x, center.y + half.y, center.z - half.z },
            Math::Vector3{ center.x - half.x, center.y + half.y, center.z - half.z },
            Math::Vector3{ center.x - half.x, center.y - half.y, center.z + half.z },
            Math::Vector3{ center.x + half.x, center.y - half.y, center.z + half.z },
            Math::Vector3{ center.x + half.x, center.y + half.y, center.z + half.z },
            Math::Vector3{ center.x - half.x, center.y + half.y, center.z + half.z }
        };

        const int edges[][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (const auto& edge : edges)
        {
            DrawLine(points[edge[0]], points[edge[1]], color);
        }
    }

    void DebugRenderer::DrawBox(const Math::Vector3& center, const Math::Vector3& size, const Math::Vector3& axisX, const Math::Vector3& axisY, const Math::Vector3& axisZ, const Math::Vector4& color)
    {
        const DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&center);
        const DirectX::XMVECTOR x = DirectX::XMVectorScale(DirectX::XMLoadFloat3(&axisX), size.x * 0.5f);
        const DirectX::XMVECTOR y = DirectX::XMVectorScale(DirectX::XMLoadFloat3(&axisY), size.y * 0.5f);
        const DirectX::XMVECTOR z = DirectX::XMVectorScale(DirectX::XMLoadFloat3(&axisZ), size.z * 0.5f);

        std::array<Math::Vector3, 8> points = {};
        const int signs[8][3] =
        {
            {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
            {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
        };

        for (std::size_t i = 0; i < points.size(); ++i)
        {
            DirectX::XMVECTOR point = c;
            point = DirectX::XMVectorAdd(point, DirectX::XMVectorScale(x, static_cast<float>(signs[i][0])));
            point = DirectX::XMVectorAdd(point, DirectX::XMVectorScale(y, static_cast<float>(signs[i][1])));
            point = DirectX::XMVectorAdd(point, DirectX::XMVectorScale(z, static_cast<float>(signs[i][2])));
            DirectX::XMStoreFloat3(&points[i], point);
        }

        const int edges[][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (const auto& edge : edges)
        {
            DrawLine(points[edge[0]], points[edge[1]], color);
        }
    }

    void DebugRenderer::DrawCircle(const Math::Vector3& center, float radius, const Math::Vector3& axisA, const Math::Vector3& axisB, const Math::Vector4& color)
    {
        constexpr int segments = 64;
        const DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&center);
        const DirectX::XMVECTOR a = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&axisA));
        const DirectX::XMVECTOR b = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&axisB));

        for (int i = 0; i < segments; ++i)
        {
            const float t0 = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
            const float t1 = DirectX::XM_2PI * static_cast<float>(i + 1) / static_cast<float>(segments);
            const DirectX::XMVECTOR p0 = DirectX::XMVectorAdd(c, DirectX::XMVectorScale(DirectX::XMVectorAdd(DirectX::XMVectorScale(a, std::cos(t0)), DirectX::XMVectorScale(b, std::sin(t0))), radius));
            const DirectX::XMVECTOR p1 = DirectX::XMVectorAdd(c, DirectX::XMVectorScale(DirectX::XMVectorAdd(DirectX::XMVectorScale(a, std::cos(t1)), DirectX::XMVectorScale(b, std::sin(t1))), radius));

            Math::Vector3 start;
            Math::Vector3 end;
            DirectX::XMStoreFloat3(&start, p0);
            DirectX::XMStoreFloat3(&end, p1);
            DrawLine(start, end, color);
        }
    }

    void DebugRenderer::DrawSphere(const Math::Vector3& center, float radius, const Math::Vector4& color)
    {
        constexpr int segments = 32;
        for (int i = 0; i < segments; ++i)
        {
            const float a0 = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
            const float a1 = DirectX::XM_2PI * static_cast<float>(i + 1) / static_cast<float>(segments);
            const float c0 = std::cos(a0);
            const float s0 = std::sin(a0);
            const float c1 = std::cos(a1);
            const float s1 = std::sin(a1);

            DrawLine({ center.x + c0 * radius, center.y, center.z + s0 * radius }, { center.x + c1 * radius, center.y, center.z + s1 * radius }, color);
            DrawLine({ center.x + c0 * radius, center.y + s0 * radius, center.z }, { center.x + c1 * radius, center.y + s1 * radius, center.z }, color);
            DrawLine({ center.x, center.y + c0 * radius, center.z + s0 * radius }, { center.x, center.y + c1 * radius, center.z + s1 * radius }, color);
        }
    }

    void DebugRenderer::Flush(RHI::RHIContext& context, const Math::Matrix& view, const Math::Matrix& projection)
    {
        if (!m_device || m_vertices.empty())
        {
            return;
        }

        std::shared_ptr<RHI::RHIBuffer> vertexBuffer = m_device->CreateVertexBuffer(
            m_vertices.data(),
            static_cast<std::uint32_t>(m_vertices.size() * sizeof(LineVertex)),
            sizeof(LineVertex));
        if (!vertexBuffer)
        {
            Clear();
            return;
        }

        DebugTransformBuffer transform = {};
        DirectX::XMStoreFloat4x4(&transform.world, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&transform.view, view);
        DirectX::XMStoreFloat4x4(&transform.projection, projection);
        context.UpdateConstantBuffer(m_transformBuffer, &transform, sizeof(transform));
        context.SetConstantBuffer(RHI::ShaderStage::Vertex, 0, m_transformBuffer);
        context.SetPrimitiveTopology(RHI::PrimitiveTopology::LineList);
        context.SetVertexBuffer(vertexBuffer);
        context.SetVertexShader(m_vertexShader);
        context.SetPixelShader(m_pixelShader);
        context.Draw(static_cast<std::uint32_t>(m_vertices.size()), 0);
        Clear();
    }

    void DebugRenderer::Clear()
    {
        m_vertices.clear();
    }
}
