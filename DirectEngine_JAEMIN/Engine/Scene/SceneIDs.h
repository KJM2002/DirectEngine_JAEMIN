#pragma once

#include <cstdint>

namespace Engine::Scene
{
    using ObjectID = std::uint64_t;
    using ComponentID = std::uint64_t;

    constexpr ObjectID InvalidObjectID = 0;
    constexpr ComponentID InvalidComponentID = 0;

    ObjectID GenerateObjectID();
    ComponentID GenerateComponentID();
    void ReserveObjectID(ObjectID id);
    void ReserveComponentID(ComponentID id);
}
