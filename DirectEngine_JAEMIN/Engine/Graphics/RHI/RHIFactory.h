#pragma once

#include "RHICommon.h"

#include <memory>

namespace Engine::RHI
{
    class RHIDevice;

    // Stage 2 exposes the entry point only. Concrete backends are plugged in later.
    std::unique_ptr<RHIDevice> CreateRHIDevice(GraphicsAPI api);
}
