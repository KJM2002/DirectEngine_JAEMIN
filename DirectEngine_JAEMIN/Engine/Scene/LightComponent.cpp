#include "LightComponent.h"

#include <algorithm>

namespace Engine::Scene
{
    void LightComponent::ClampConeAngles()
    {
        innerConeAngle = std::clamp(innerConeAngle, 0.0f, 179.0f);
        outerConeAngle = std::clamp(outerConeAngle, innerConeAngle, 179.0f);
    }
}
