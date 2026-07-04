#pragma once

#include <string>

namespace Engine::Asset
{
    struct AssetReference
    {
        std::string guid;

        bool IsValid() const
        {
            return !guid.empty();
        }
    };
}
