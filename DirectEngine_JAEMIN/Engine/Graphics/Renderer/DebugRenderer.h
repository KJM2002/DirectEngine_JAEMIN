#pragma once

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

namespace Engine::Renderer
{
    class DebugRenderer
    {
    public:
        bool Initialize(RHI::RHIDevice& device);
        void Shutdown();
        void DrawLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color);
        void DrawAABB(const Math::Vector3& center, const Math::Vector3& size, const Math::Vector4& color);
        void DrawBox(const Math::Vector3& center, const Math::Vector3& size, const Math::Vector3& axisX, const Math::Vector3& axisY, const Math::Vector3& axisZ, const Math::Vector4& color);
        void DrawCircle(const Math::Vector3& center, float radius, const Math::Vector3& axisA, const Math::Vector3& axisB, const Math::Vector4& color);
        void DrawSphere(const Math::Vector3& center, float radius, const Math::Vector4& color);
        void Flush(RHI::RHIContext& context, const Math::Matrix& view, const Math::Matrix& projection);
        void Clear();

    private:
        struct LineVertex
        {
            float position[3] = {};
            float normal[3] = {};
            float color[4] = {};
            float uv[2] = {};
        };

        struct DebugTransformBuffer
        {
            Math::Matrix4x4 world;
            Math::Matrix4x4 view;
            Math::Matrix4x4 projection;
        };

        RHI::RHIDevice* m_device = nullptr;
        std::shared_ptr<RHI::RHIShader> m_vertexShader;
        std::shared_ptr<RHI::RHIShader> m_pixelShader;
        std::shared_ptr<RHI::RHIBuffer> m_transformBuffer;
        std::vector<LineVertex> m_vertices;
    };
}
