#pragma once

#include <cstdint>

namespace Engine::RHI
{
    // Owns presentation-related resources without exposing DXGI or platform headers.
    class RHISwapChain
    {
    public:
        virtual ~RHISwapChain() = default;

        virtual bool Initialize(void* nativeWindowHandle, std::uint32_t width, std::uint32_t height) = 0;
        virtual void Shutdown() = 0;
        virtual void Present() = 0;
        virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    };
}
