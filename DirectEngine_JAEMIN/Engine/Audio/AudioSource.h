#pragma once

#include "AudioClip.h"

#include <memory>

namespace Engine::Audio
{
    class AudioSource
    {
    public:
        void SetClip(std::shared_ptr<AudioClip> clip);
        const std::shared_ptr<AudioClip>& GetClip() const;

        void SetLooping(bool looping);
        bool IsLooping() const;

        void SetVolume(float volume);
        float GetVolume() const;

        void Play();
        void Stop();
        bool IsPlaying() const;

    private:
        std::shared_ptr<AudioClip> m_clip;
        float m_volume = 1.0f;
        bool m_looping = false;
        bool m_playing = false;
    };
}
