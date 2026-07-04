#include "Time.h"

namespace Engine::Core
{
    void Time::Reset()
    {
        m_lastTime = std::chrono::steady_clock::now();
        m_deltaSeconds = 0.0;
        m_totalSeconds = 0.0;
    }

    void Time::Update()
    {
        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = currentTime - m_lastTime;
        m_lastTime = currentTime;

        m_deltaSeconds = elapsed.count();
        m_totalSeconds += m_deltaSeconds;
    }

    float Time::GetDeltaTime() const
    {
        return static_cast<float>(m_deltaSeconds);
    }

    float Time::GetTotalTime() const
    {
        return static_cast<float>(m_totalSeconds);
    }
}
