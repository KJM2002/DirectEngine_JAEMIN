#pragma once

#include "ICommand.h"
#include "GameObjectSnapshot.h"

namespace Engine::Scene
{
    class GameObject;
}

namespace Engine::Editor
{
    class InspectorEditCommand : public ICommand
    {
    public:
        InspectorEditCommand(Scene::GameObject& object, const GameObjectSnapshot& before, const GameObjectSnapshot& after);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

    private:
        Scene::GameObject& m_object;
        GameObjectSnapshot m_before;
        GameObjectSnapshot m_after;
    };
}
