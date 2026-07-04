#pragma once

#include "ICommand.h"
#include "GameObjectSnapshot.h"

#include <vector>

namespace Engine::Scene
{
    class GameObject;
    class Scene;
}

namespace Engine::Editor
{
    class DeleteGameObjectCommand : public ICommand
    {
    public:
        DeleteGameObjectCommand(Scene::Scene& scene, const std::vector<Scene::GameObject*>& objects);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

    private:
        Scene::Scene& m_scene;
        std::vector<GameObjectSnapshot> m_snapshots;
        std::vector<Scene::GameObject*> m_objects;
    };
}
