#pragma once

#include "Engine/Math/MathTypes.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIContext;
    class RHIDevice;
    class RHIShader;
    class RHITexture;
}

namespace Engine::Scene
{
    class Camera;
    class GameObject;
}

namespace Engine::Renderer
{
    struct SelectionOutlineSettings;

    class SelectionOutlineRenderer
    {
    public:
        bool Initialize(RHI::RHIDevice& device, std::uint32_t width, std::uint32_t height);
        void Shutdown();
        bool Resize(std::uint32_t width, std::uint32_t height);

        bool RenderMask(
            RHI::RHIContext& context,
            const std::vector<Scene::GameObject*>& selectedObjects,
            const Scene::Camera& camera,
            float aspectRatio,
            const std::shared_ptr<RHI::RHITexture>& depthTarget);

        void Composite(
            RHI::RHIContext& context,
            const std::shared_ptr<RHI::RHITexture>& sceneColor,
            const std::shared_ptr<RHI::RHITexture>& outputTarget,
            const SelectionOutlineSettings& settings,
            bool maskReady);

        void CompositeToDefault(
            RHI::RHIContext& context,
            const std::shared_ptr<RHI::RHITexture>& sceneColor,
            const SelectionOutlineSettings& settings,
            bool maskReady);

        const std::shared_ptr<RHI::RHITexture>& GetCompositeTarget() const;

    private:
        struct FullscreenVertex
        {
            Math::Vector3 position;
            Math::Vector3 normal;
            Math::Vector4 color;
            Math::Vector2 uv;
            Math::Vector3 tangent;
        };

        struct TransformBufferData
        {
            Math::Matrix4x4 world;
            Math::Matrix4x4 view;
            Math::Matrix4x4 projection;
        };

        struct CompositeBufferData
        {
            Math::Vector4 outlineColor;
            Math::Vector2 texelSize;
            float outlineWidth = 4.0f;
            float opacity = 1.0f;
            std::int32_t maskReady = 0;
            float padding[3] = {};
        };

        bool CreateTargets(std::uint32_t width, std::uint32_t height);
        void DrawComposite(
            RHI::RHIContext& context,
            const std::shared_ptr<RHI::RHITexture>& sceneColor,
            const SelectionOutlineSettings& settings,
            bool maskReady);

        RHI::RHIDevice* m_device = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::shared_ptr<RHI::RHITexture> m_maskTarget;
        std::shared_ptr<RHI::RHITexture> m_compositeTarget;
        std::shared_ptr<RHI::RHIBuffer> m_fullscreenVertexBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_fullscreenIndexBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_transformBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_compositeBuffer;
        std::shared_ptr<RHI::RHIShader> m_maskVertexShader;
        std::shared_ptr<RHI::RHIShader> m_maskPixelShader;
        std::shared_ptr<RHI::RHIShader> m_fullscreenVertexShader;
        std::shared_ptr<RHI::RHIShader> m_outlinePixelShader;
    };
}
