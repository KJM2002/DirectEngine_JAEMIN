#pragma once

#include "Component.h"
#include "Light.h"
#include "Engine/Math/MathTypes.h"

namespace Engine::Scene
{
    class LightComponent : public Component
    {
    public:
        static constexpr const char* StaticTypeName = "Light";
        const char* GetTypeName() const override { return StaticTypeName; }

        LightType type = LightType::Point;
        Math::Vector3 color = { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float range = 5.0f;
        Math::Vector3 direction = { 0.0f, -1.0f, 0.0f };
        float innerConeAngle = 20.0f;
        float outerConeAngle = 35.0f;
        Math::Vector3 ambientColor = { 0.0f, 0.0f, 0.0f };

        void ClampConeAngles();
    };
}
