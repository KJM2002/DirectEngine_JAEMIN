#pragma once

#include "Engine/Graphics/RHI/RHICommon.h"

#include <cstdint>
#include <vector>

namespace Engine::Resource
{
    // CPU-side decoded image data. Graphics backends consume this through RHI.
    struct ImageData
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t channelCount = 4;
        RHI::TextureFormat format = RHI::TextureFormat::RGBA8;
        std::vector<std::uint8_t> pixels;

        bool IsValid() const
        {
            return width > 0 && height > 0 && channelCount > 0 && !pixels.empty();
        }

        std::uint32_t GetRowPitch() const
        {
            return width * channelCount;
        }
    };
}
