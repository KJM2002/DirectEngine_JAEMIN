#pragma once

#include "GameObject.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Engine::Renderer
{
    class Renderer;
}

namespace Engine::Scene
{
    class Camera;
    class DirectionalLight;

    class Scene
    {
    public:
        GameObject& CreateGameObject(const std::string& name);
        bool DestroyGameObject(GameObject* object);
        void Update(float deltaTime);
        void Render(Renderer::Renderer& renderer);
        void Clear();
        void SetActiveCamera(std::shared_ptr<Camera> camera);
        const std::shared_ptr<Camera>& GetActiveCamera() const;
        void SetDirectionalLight(std::shared_ptr<DirectionalLight> light);
        const std::shared_ptr<DirectionalLight>& GetDirectionalLight() const;
        void EnsureDefaultCameraAndLight();
        bool IsDirty() const;
        void MarkDirty();
        void ClearDirty();
        const std::filesystem::path& GetFilePath() const;
        void SetFilePath(const std::filesystem::path& path);
        std::string GetDisplayName() const;
        std::string GetDirtyDisplayName() const;

        const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const;

    private:
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;
        std::shared_ptr<Camera> m_activeCamera;
        std::shared_ptr<DirectionalLight> m_directionalLight;
        std::filesystem::path m_filePath;
        bool m_dirty = false;
    };
}
