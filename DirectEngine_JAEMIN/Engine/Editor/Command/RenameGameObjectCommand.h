#pragma once

#include "ICommand.h"

#include <string>

namespace Engine::Scene
{
    class GameObject;
}

namespace Engine::Editor
{
    class RenameGameObjectCommand : public ICommand
    {
    public:
        RenameGameObjectCommand(Scene::GameObject& object, std::string oldName, std::string newName);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

    private:
        Scene::GameObject& m_object;
        std::string m_oldName;
        std::string m_newName;
    };
}
