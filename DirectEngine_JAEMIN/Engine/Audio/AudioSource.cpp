#include "AudioSource.h"

#include <algorithm>

namespace Engine::Audio
{
    void AudioSource::SetClip(std::shared_ptr<AudioClip> clip)
    {
        m_clip = std::move(clip);
    }

    const std::shared_ptr<AudioClip>& AudioSource::GetClip() const
    {
        return m_clip;
    }

    void AudioSource::SetLooping(bool looping)
    {
        m_looping = looping;
    }

    bool AudioSource::IsLooping() const
    {
        return m_looping;
    }

    void AudioSource::SetVolume(float volume)
    {
        m_volume = std::clamp(volume, 0.0f, 1.0f);
    }

    float AudioSource::GetVolume() const
    {
        return m_volume;
    }

    void AudioSource::Play()
    {
        m_playing = m_clip != nullptr;
    }

    void AudioSource::Stop()
    {
        m_playing = false;
    }

    bool AudioSource::IsPlaying() const
    {
        return m_playing;
    }
}
