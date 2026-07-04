#pragma once

#include <string>

namespace Engine::Resource
{
    class ResourceManager;
}

namespace Engine::Scene
{
    class Scene;

    class SceneSerializer
    {
    public:
        static bool LoadScene(const std::wstring& path, Scene& scene, Resource::ResourceManager& resourceManager);
        static bool SaveScene(const std::wstring& path, const Scene& scene);
    };
}
