#include "GuidGenerator.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>

namespace Engine::Asset
{
    std::string GuidGenerator::NewGuid()
    {
        static thread_local std::mt19937_64 rng(
            static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
            ^ std::random_device{}());

        std::array<std::uint64_t, 2> values = { rng(), rng() };
        std::ostringstream stream;
        stream << std::hex << std::setfill('0')
               << std::setw(16) << values[0]
               << std::setw(16) << values[1];
        return stream.str();
    }
}
