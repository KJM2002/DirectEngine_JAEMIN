#pragma once

#include "RHICommon.h"

#include <cstdint>

namespace Engine::RHI
{
    // API-independent texture object. Concrete backends map this to their own image type.
    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;

        virtual TextureFormat GetFormat() const = 0;
        virtual std::uint32_t GetWidth() const = 0;
        virtual std::uint32_t GetHeight() const = 0;
        virtual void* GetNativeShaderResourceHandleForUI() const { return nullptr; }
    };
}
