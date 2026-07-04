#pragma once

#include "Engine/Math/MathTypes.h"

namespace Engine::Physics
{
    struct Ray
    {
        Math::Vector3 origin = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 direction = { 0.0f, 0.0f, 1.0f };
    };
}
