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
    class DuplicateGameObjectCommand : public ICommand
    {
    public:
        DuplicateGameObjectCommand(Scene::Scene& scene, const std::vector<Scene::GameObject*>& sources);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

        const std::vector<Scene::GameObject*>& GetDuplicates() const;

    private:
        std::string MakeUniqueName(const std::string& sourceName) const;

        Scene::Scene& m_scene;
        std::vector<GameObjectSnapshot> m_snapshots;
        std::vector<Scene::GameObject*> m_duplicates;
    };
}
