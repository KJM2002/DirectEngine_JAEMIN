#pragma once

#include "Engine/Math/MathTypes.h"

namespace Engine::Scene
{
    class GameObject;
}

namespace Engine::Physics
{
    struct RaycastHit
    {
        Scene::GameObject* object = nullptr;
        Math::Vector3 point = { 0.0f, 0.0f, 0.0f };
        float distance = 0.0f;
    };
}
