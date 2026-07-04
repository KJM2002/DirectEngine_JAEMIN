#pragma once

#include "Component.h"

namespace Engine::Scene
{
    class PlayerStartComponent : public Component
    {
    public:
        float playerHeight = 1.7f;
        float moveSpeed = 4.0f;
        float fastMoveMultiplier = 3.0f;
        float mouseSensitivity = 0.003f;
    };
}
