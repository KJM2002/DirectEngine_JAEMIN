#include "AudioSystem.h"

#include "Engine/Core/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>

#include <algorithm>

namespace Engine::Audio
{
    bool AudioSystem::Initialize()
    {
        m_initialized = true;
        Core::Log::Info("AudioSystem initialized. Real WAV playback backend can be added without changing game code.");
        return true;
    }

    void AudioSystem::Shutdown()
    {
        StopAll();
        m_sources.clear();
        m_clips.clear();
        m_initialized = false;
    }

    void AudioSystem::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    std::shared_ptr<AudioClip> AudioSystem::LoadClip(const std::wstring& path)
    {
        auto existing = std::find_if(m_clips.begin(), m_clips.end(), [&path](const std::shared_ptr<AudioClip>& clip)
        {
            return clip && clip->GetPath() == path;
        });

        if (existing != m_clips.end())
        {
            return *existing;
        }

        std::shared_ptr<AudioClip> clip = std::make_shared<AudioClip>(path, 0.0f);
        m_clips.push_back(clip);
        return clip;
    }

    std::shared_ptr<AudioSource> AudioSystem::CreateSource()
    {
        std::shared_ptr<AudioSource> source = std::make_shared<AudioSource>();
        m_sources.push_back(source);
        return source;
    }

    void AudioSystem::PlayOneShot(const std::shared_ptr<AudioClip>& clip, float volume)
    {
        if (!m_initialized || !clip)
        {
            return;
        }

        std::shared_ptr<AudioSource> source = CreateSource();
        source->SetClip(clip);
        source->SetVolume(volume);
        source->Play();

        if (!PlaySoundW(clip->GetPath().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT))
        {
            Core::Log::Warning("Failed to play WAV file through WinMM.");
            source->Stop();
        }
    }

    void AudioSystem::StopAll()
    {
        PlaySoundW(nullptr, nullptr, 0);
        for (const std::shared_ptr<AudioSource>& source : m_sources)
        {
            if (source)
            {
                source->Stop();
            }
        }
    }
}
