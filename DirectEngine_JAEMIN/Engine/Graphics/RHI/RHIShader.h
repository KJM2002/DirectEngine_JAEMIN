#pragma once

#include "RHICommon.h"

namespace Engine::RHI
{
    // API-independent shader object. Backends own the compiled/native shader data.
    class RHIShader
    {
    public:
        virtual ~RHIShader() = default;

        virtual ShaderStage GetStage() const = 0;
    };
}
