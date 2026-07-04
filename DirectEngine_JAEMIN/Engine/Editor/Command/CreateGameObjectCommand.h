#pragma once

#include "ICommand.h"
#include "GameObjectSnapshot.h"

namespace Engine::Scene
{
    class GameObject;
    class Scene;
}

namespace Engine::Editor
{
    class CreateGameObjectCommand : public ICommand
    {
    public:
        CreateGameObjectCommand(Scene::Scene& scene, GameObjectSnapshot snapshot);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

        Scene::GameObject* GetCreatedObject() const;

    private:
        Scene::Scene& m_scene;
        GameObjectSnapshot m_snapshot;
        Scene::GameObject* m_object = nullptr;
    };
}
