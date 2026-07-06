#include "Application.h"

#include "Log.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Scene/Light.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/Serialization/SceneSerializer.h"
#include "Game/GameApp.h"

#include <memory>
#include <utility>

namespace Engine::Core
{
    bool Application::Initialize()
    {
        constexpr std::uint32_t width = 1280;
        constexpr std::uint32_t height = 720;

        Log::Info("Initializing application.");

        if (!m_window.Create(L"DirectEngine JAEMIN", width, height))
        {
            Log::Error("Failed to create Win32 window.");
            return false;
        }

        if (!m_renderer.Initialize(RHI::GraphicsAPI::D3D11, m_window.GetNativeHandle(), width, height))
        {
            Log::Error("Failed to initialize renderer.");
            return false;
        }

        if (!m_audioSystem.Initialize())
        {
            Log::Error("Failed to initialize audio system.");
            return false;
        }

        if (!InitializeScene())
        {
            Log::Error("Failed to initialize scene.");
            return false;
        }

        if (!m_debugUI.Initialize())
        {
            Log::Error("Failed to initialize debug UI.");
            return false;
        }

        if (!m_editorLayer.Initialize())
        {
            Log::Error("Failed to initialize editor layer.");
            return false;
        }

        if (!m_imguiDebugUI.Initialize(
            m_window.GetNativeHandle(),
            m_renderer.GetNativeDeviceHandleForDebugUI(),
            m_renderer.GetNativeContextHandleForDebugUI()))
        {
            Log::Error("Failed to initialize ImGui debug UI.");
            return false;
        }

        m_time.Reset();
        m_isRunning = true;
        return true;
    }

    int Application::Run()
    {
        Game::GameApp game;

        while (m_isRunning)
        {
            m_input.BeginFrame();
            m_isRunning = m_window.ProcessMessages();
            if (!m_isRunning)
            {
                break;
            }
            if (m_window.ConsumeCloseRequest())
            {
                m_editorLayer.RequestExit(m_scene);
            }

            std::uint32_t resizedWidth = 0;
            std::uint32_t resizedHeight = 0;
            if (m_window.ConsumeResizeEvent(resizedWidth, resizedHeight))
            {
                m_renderer.Resize(resizedWidth, resizedHeight);
            }

            m_time.Update();
            game.Update(m_time.GetDeltaTime());

            m_audioSystem.Update(m_time.GetDeltaTime());
            Update(m_time.GetDeltaTime());
            Render();
        }

        Log::Info("Application loop finished.");
        return 0;
    }

    void Application::Update(float deltaTime)
    {
        SynchronizeSceneReferences();

        if (m_input.GetKeyDown(Input::KeyEscape))
        {
            m_editorLayer.RequestExit(m_scene);
        }

        m_editorLayer.Update(m_scene, m_debugStats, m_input, deltaTime);
        if (m_editorLayer.ConsumeExitRequested())
        {
            RequestQuit();
            return;
        }
        const bool playMode = m_editorLayer.IsPlayMode();
        if (!m_wasPlayMode && playMode)
        {
            EnterPlayMode();
        }
        else if (m_wasPlayMode && !playMode)
        {
            ExitPlayMode();
        }
        m_wasPlayMode = playMode;

        const bool rightMouseHeld = m_input.GetMouseButtonRaw(Input::MouseButtonRight);
        m_input.SetInputBlocked(m_editorLayer.WantsInputCapture() && !rightMouseHeld);

        if (m_camera && !playMode)
        {
            m_input.SetMouseCaptured(rightMouseHeld);
            m_editorCamera.Update(m_input, deltaTime);
            m_editorCamera.ApplyTo(*m_camera);
        }
        else if (m_camera)
        {
            m_input.SetMouseCaptured(true);
            m_camera->UpdateFromInput(m_input, deltaTime);
        }

        if (m_demoObject)
        {
            m_demoObject->GetTransform().rotation.x += deltaTime * 0.75f;
            m_demoObject->GetTransform().rotation.y += deltaTime * 1.15f;
        }

        m_scene.Update(deltaTime);

        Physics::PhysicsWorld physicsWorld;
        physicsWorld.BuildFromScene(m_scene);
        (void)physicsWorld.UpdateOverlaps();
    }

    void Application::Render()
    {
        m_editorLayer.ApplyRendererSettings(m_renderer, m_scene);
        m_renderer.RenderShadowPass(m_scene);
        m_renderer.BeginFrame(0.08f, 0.08f, 0.09f, 1.0f);
        m_imguiDebugUI.BeginFrame();
        m_scene.Render(m_renderer);
        m_renderer.RenderColliders(m_scene, m_editorLayer.GetSelectedObject(), m_editorLayer.ShouldShowColliders());
        m_renderer.RenderSelectionOutlines(m_editorLayer.GetSelectedObjects());
        m_renderer.RenderSelectionGizmo(m_editorLayer.GetSelectedObject(), m_editorLayer.ShouldShowGizmo(), m_editorLayer.GetGizmoVisualMode());
        m_renderer.ApplyPostProcess();
        UpdateDebugStats(m_time.GetDeltaTime());
        m_editorLayer.Render(m_scene, m_debugStats, m_renderer);
        SynchronizeSceneReferences();
        m_imguiDebugUI.EndFrame();
        m_renderer.EndFrame();
    }

    void Application::Shutdown()
    {
        Log::Info("Shutting down application.");
        m_demoObject = nullptr;
        m_scene.Clear();
        m_imguiDebugUI.Shutdown();
        m_editorLayer.Shutdown();
        m_debugUI.Shutdown();
        m_audioSystem.Shutdown();
        m_renderer.Shutdown();
        m_window.Destroy();
        m_isRunning = false;
    }

    void Application::RequestQuit()
    {
        m_isRunning = false;
    }

    bool Application::InitializeScene()
    {
        Resource::ResourceManager& resources = m_renderer.GetResourceManager();
        if (Scene::SceneSerializer::LoadScene(L"Assets/Scenes/Test.scene", m_scene, resources))
        {
            m_camera = m_scene.GetActiveCamera();
            if (m_camera)
            {
                m_editorCamera.SetFrom(*m_camera);
            }
            m_scene.SetFilePath(L"Assets/Scenes/Test.scene");
            m_scene.ClearDirty();
            m_demoObject = nullptr;
            return true;
        }

        m_camera = std::make_shared<Scene::Camera>();
        m_scene.SetActiveCamera(m_camera);
        m_editorCamera.SetFrom(*m_camera);

        auto light = std::make_shared<Scene::DirectionalLight>();
        light->direction = { 0.4f, -1.0f, 0.35f };
        light->color = { 1.0f, 0.95f, 0.85f };
        light->ambientColor = { 0.18f, 0.20f, 0.24f };
        light->intensity = 1.0f;
        m_scene.SetDirectionalLight(light);

        std::shared_ptr<Renderer::Mesh> cubeMesh = resources.CreateCubeMesh("builtin:mesh:cube");
        if (!cubeMesh)
        {
            Log::Error("Failed to create cube mesh.");
            return false;
        }

        std::shared_ptr<Renderer::Material> cubeMaterial = resources.CreateMaterial("builtin:material:checker_cube", { 1.0f, 1.0f, 1.0f, 1.0f });
        if (!cubeMaterial)
        {
            Log::Error("Failed to create cube material.");
            return false;
        }

        Scene::GameObject& cube = m_scene.CreateGameObject("Cube");
        Scene::MeshRendererComponent& meshRenderer = cube.AddComponent<Scene::MeshRendererComponent>();
        meshRenderer.SetMesh(std::move(cubeMesh));
        meshRenderer.SetMaterial(std::move(cubeMaterial));
        meshRenderer.SetMeshPath(L"builtin:mesh:cube");
        meshRenderer.SetMaterialPath(L"builtin:material:checker_cube");
        m_demoObject = &cube;

        Scene::GameObject& playerStart = m_scene.CreateGameObject("PlayerStart");
        playerStart.GetTransform().position = { 0.0f, 0.0f, -5.0f };
        playerStart.AddComponent<Scene::PlayerStartComponent>();
        m_scene.SetFilePath({});
        m_scene.ClearDirty();
        return true;
    }

    Scene::GameObject* Application::FindPlayerStartObject() const
    {
        for (const std::unique_ptr<Scene::GameObject>& object : m_scene.GetGameObjects())
        {
            if (object && object->GetComponent<Scene::PlayerStartComponent>())
            {
                return object.get();
            }
        }

        return nullptr;
    }

    void Application::EnterPlayMode()
    {
        if (!m_camera)
        {
            m_camera = std::make_shared<Scene::Camera>();
            m_scene.SetActiveCamera(m_camera);
        }

        m_savedEditorCamera = *m_camera;
        m_hasSavedEditorCamera = true;

        if (Scene::GameObject* playerStartObject = FindPlayerStartObject())
        {
            const Scene::PlayerStartComponent* playerStart = playerStartObject->GetComponent<Scene::PlayerStartComponent>();
            const Math::Transform& transform = playerStartObject->GetTransform();
            m_camera->position = transform.position;
            if (playerStart)
            {
                m_camera->position.y += playerStart->playerHeight;
                m_camera->moveSpeed = playerStart->moveSpeed;
                m_camera->fastMoveMultiplier = playerStart->fastMoveMultiplier;
                m_camera->mouseSensitivity = playerStart->mouseSensitivity;
            }
            m_camera->pitch = transform.rotation.x;
            m_camera->yaw = transform.rotation.y;
            Log::Info("Play Mode started at PlayerStart.");
        }
        else
        {
            Log::Warning("Play Mode started without PlayerStart; using current camera.");
        }

        m_input.SetMouseCaptured(true);
    }

    void Application::ExitPlayMode()
    {
        if (m_camera && m_hasSavedEditorCamera)
        {
            *m_camera = m_savedEditorCamera;
            m_editorCamera.SetFrom(*m_camera);
        }

        m_hasSavedEditorCamera = false;
        m_input.SetMouseCaptured(false);
        Log::Info("Returned to Editor Mode.");
    }

    void Application::SynchronizeSceneReferences()
    {
        const std::shared_ptr<Scene::Camera>& activeCamera = m_scene.GetActiveCamera();
        if (activeCamera && activeCamera != m_camera)
        {
            m_camera = activeCamera;
            m_editorCamera.SetFrom(*m_camera);
            m_demoObject = nullptr;
            m_input.SetMouseCaptured(false);
            Log::Info("Synchronized application camera after scene change.");
        }
        else if (!activeCamera)
        {
            m_camera.reset();
            m_demoObject = nullptr;
        }
    }

    void Application::UpdateDebugStats(float deltaTime)
    {
        m_debugStats.deltaTime = deltaTime;
        m_debugStats.fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
        m_debugStats.objectCount = static_cast<std::uint32_t>(m_scene.GetGameObjects().size());
        m_debugStats.drawCallCount = m_renderer.GetDrawCallCount();
        m_debugStats.triangleCount = m_renderer.GetTriangleCount();
        m_debugStats.cameraPosition = m_camera ? m_camera->position : Math::Vector3{};

        if (m_debugUI.Update(m_debugStats))
        {
            m_window.SetTitle(m_editorLayer.BuildWindowTitle(m_debugUI.GetWindowTitle(), m_scene));
        }
    }
}
