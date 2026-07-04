#pragma once

#include <string>

namespace Engine::Audio
{
    class AudioClip
    {
    public:
        AudioClip() = default;
        AudioClip(std::wstring path, float durationSeconds);

        const std::wstring& GetPath() const;
        float GetDurationSeconds() const;

    private:
        std::wstring m_path;
        float m_durationSeconds = 0.0f;
    };
}
