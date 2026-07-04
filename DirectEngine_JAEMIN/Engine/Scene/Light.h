#pragma once

#include "Engine/Math/MathTypes.h"

#include <string>

namespace Engine::Scene
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    class DirectionalLight
    {
    public:
        std::string name = "Sun";
        Math::Vector3 direction = { 0.5f, -1.0f, 0.5f };
        Math::Vector3 color = { 1.0f, 0.95f, 0.85f };
        Math::Vector3 ambientColor = { 0.18f, 0.20f, 0.24f };
        float intensity = 1.0f;
    };
}
