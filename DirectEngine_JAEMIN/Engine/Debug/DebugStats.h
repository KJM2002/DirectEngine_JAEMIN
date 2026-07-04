#pragma once

#include "Engine/Math/MathTypes.h"

#include <cstdint>

namespace Engine::Debug
{
    struct DebugStats
    {
        float fps = 0.0f;
        float deltaTime = 0.0f;
        std::uint32_t objectCount = 0;
        std::uint32_t drawCallCount = 0;
        std::uint32_t triangleCount = 0;
        Math::Vector3 cameraPosition = { 0.0f, 0.0f, 0.0f };
    };
}
