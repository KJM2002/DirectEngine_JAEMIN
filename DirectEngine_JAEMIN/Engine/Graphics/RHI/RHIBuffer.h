#pragma once

#include "RHICommon.h"

namespace Engine::RHI
{
    // API-independent handle for vertex, index, and constant buffers.
    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;

        virtual const BufferDesc& GetDesc() const = 0;
        virtual BufferType GetType() const = 0;
        virtual std::uint32_t GetSize() const = 0;
        virtual std::uint32_t GetStride() const = 0;
        virtual std::uint32_t GetCount() const = 0;
    };
}
