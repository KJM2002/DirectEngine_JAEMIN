#pragma once

#include "Engine/Core/Time.h"
#include "Engine/Audio/AudioSystem.h"
#include "Engine/Debug/DebugStats.h"
#include "Engine/Debug/DebugUI.h"
#include "Engine/Debug/ImGuiDebugUI.h"
#include "Engine/Editor/EditorCamera.h"
#include "Engine/Editor/EditorLayer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Input/Input.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Platform/Windows/Win32Window.h"
#include "Engine/Scene/Scene.h"

namespace Engine::Core
{
    // Owns the high-level lifetime: create window, run loop, update, render, shutdown.
    class Application
    {
    public:
        bool Initialize();
        int Run();
        void Update(float deltaTime);
        void Render();
        void Shutdown();
        void RequestQuit();

    private:
        bool InitializeScene();
        Scene::GameObject* FindPlayerStartObject() const;
        void EnterPlayMode();
        void ExitPlayMode();
        void SynchronizeSceneReferences();
        void UpdateDebugStats(float deltaTime);

        Platform::Win32Window m_window;
        Renderer::Renderer m_renderer;
        Scene::Scene m_scene;
        std::shared_ptr<Scene::Camera> m_camera;
        Editor::EditorCamera m_editorCamera;
        Editor::EditorLayer m_editorLayer;
        Debug::ImGuiDebugUI m_imguiDebugUI;
        Debug::DebugUI m_debugUI;
        Debug::DebugStats m_debugStats;
        Audio::AudioSystem m_audioSystem;
        Input::Input m_input;
        Time m_time;
        bool m_isRunning = false;
        bool m_wasPlayMode = false;
        bool m_hasSavedEditorCamera = false;
        Scene::Camera m_savedEditorCamera;
    };
}
