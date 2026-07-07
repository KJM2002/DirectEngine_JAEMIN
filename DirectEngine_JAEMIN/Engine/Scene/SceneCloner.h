#pragma once

#include <memory>

namespace Engine::Scene
{
    class Scene;

    class SceneCloner
    {
    public:
        static std::unique_ptr<Scene> Clone(const Scene& source);
        static void CloneInto(const Scene& source, Scene& target);
    };
}
