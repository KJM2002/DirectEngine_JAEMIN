#pragma once

#include "Engine/Math/MathTypes.h"

#include <string>
#include <variant>

namespace Engine::Editor
{
    using PropertyValue = std::variant<float, bool, int, Math::Vector3, Math::Vector4, std::string>;
}
