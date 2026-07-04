#pragma once

#include "AudioClip.h"
#include "AudioSource.h"

#include <memory>
#include <string>
#include <vector>

namespace Engine::Audio
{
    class AudioSystem
    {
    public:
        bool Initialize();
        void Shutdown();
        void Update(float deltaTime);

        std::shared_ptr<AudioClip> LoadClip(const std::wstring& path);
        std::shared_ptr<AudioSource> CreateSource();
        void PlayOneShot(const std::shared_ptr<AudioClip>& clip, float volume = 1.0f);
        void StopAll();

    private:
        std::vector<std::shared_ptr<AudioClip>> m_clips;
        std::vector<std::shared_ptr<AudioSource>> m_sources;
        bool m_initialized = false;
    };
}
