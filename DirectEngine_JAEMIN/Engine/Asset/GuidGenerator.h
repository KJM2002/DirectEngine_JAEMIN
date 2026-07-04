#pragma once

#include <string>

namespace Engine::Asset
{
    class GuidGenerator
    {
    public:
        static std::string NewGuid();
    };
}
