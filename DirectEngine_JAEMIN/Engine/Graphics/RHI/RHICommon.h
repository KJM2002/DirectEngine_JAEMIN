#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace Engine::RHI
{
    // Backend selector used by the factory. It is not tied to any platform header.
    enum class GraphicsAPI
    {
        D3D11,
        D3D12
    };

    enum class Result
    {
        Success,
        Failed
    };

    enum class ResourceState
    {
        Unknown,
        Common,
        RenderTarget,
        DepthWrite,
        ShaderResource,
        Present,
        CopySource,
        CopyDest
    };

    enum class DescriptorType
    {
        Unknown,
        RenderTarget,
        DepthStencil,
        ShaderResource,
        ConstantBuffer,
        Sampler
    };

    struct CommandListDesc
    {
        const char* debugName = nullptr;
    };

    // Generic buffer lifetime hint. Concrete APIs translate this to their own usage flags.
    enum class BufferUsage
    {
        Static,
        Dynamic
    };

    // What the buffer will be bound as during rendering.
    enum class BufferType
    {
        Vertex,
        Index,
        Constant
    };

    // Shader stages supported by the first renderer milestone.
    enum class ShaderStage
    {
        Vertex,
        Pixel
    };

    // Engine-level formats. D3D11/D3D12/Vulkan values must be mapped in each backend.
    enum class TextureFormat
    {
        Unknown,
        RGBA8,
        BGRA8,
        RGBA8_sRGB,
        D24S8,
        D32
    };

    enum class TextureFilter
    {
        Point,
        Linear,
        ComparisonLinear
    };

    enum class TextureAddressMode
    {
        Wrap,
        Clamp,
        Border
    };

    // Minimal primitive set for the early renderer.
    enum class PrimitiveTopology
    {
        TriangleList,
        LineList
    };

    // Shared creation data for buffers without exposing API-specific descriptors.
    struct BufferDesc
    {
        BufferUsage usage = BufferUsage::Static;
        BufferType type = BufferType::Vertex;
        std::size_t byteSize = 0;
        std::uint32_t stride = 0;
        std::uint32_t count = 0;
        bool dynamic = false;
    };

    // Shared texture creation data for CPU-provided image pixels.
    struct TextureDesc
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        TextureFormat format = TextureFormat::RGBA8;
        const void* initialData = nullptr;
        std::uint32_t rowPitch = 0;
        bool shaderResource = true;
        bool renderTarget = false;
        bool depthStencil = false;
        bool generateMips = false;
        TextureFilter filter = TextureFilter::Linear;
        TextureAddressMode addressMode = TextureAddressMode::Wrap;
    };

    // Shared shader creation data. The file format is backend-agnostic at this layer.
    struct ShaderDesc
    {
        ShaderStage stage = ShaderStage::Vertex;
        std::wstring path;
        std::string entryPoint = "main";
        std::string target;
    };

    // The native window handle remains opaque so RHI does not include Windows.h.
    struct SwapChainDesc
    {
        void* nativeWindowHandle = nullptr;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool enableVSync = true;
    };

    // Viewport state expressed in engine terms.
    struct ViewportDesc
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };
}
