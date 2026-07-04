#include "AudioClip.h"

#include <utility>

namespace Engine::Audio
{
    AudioClip::AudioClip(std::wstring path, float durationSeconds)
        : m_path(std::move(path))
        , m_durationSeconds(durationSeconds)
    {
    }

    const std::wstring& AudioClip::GetPath() const
    {
        return m_path;
    }

    float AudioClip::GetDurationSeconds() const
    {
        return m_durationSeconds;
    }
}
