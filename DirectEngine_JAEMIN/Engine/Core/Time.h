#pragma once

#include <chrono>

namespace Engine::Core
{
    // Frame timer based on the C++ standard clock so Core stays independent from Win32.
    class Time
    {
    public:
        void Reset();
        void Update();

        float GetDeltaTime() const;
        float GetTotalTime() const;

    private:
        double m_deltaSeconds = 0.0;
        double m_totalSeconds = 0.0;
        std::chrono::steady_clock::time_point m_lastTime;
    };
}
