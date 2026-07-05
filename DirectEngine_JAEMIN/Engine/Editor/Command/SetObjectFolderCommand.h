#pragma once

#include "ICommand.h"

#include <string>
#include <vector>

namespace Engine::Scene
{
    class GameObject;
}

namespace Engine::Editor
{
    class SetObjectFolderCommand : public ICommand
    {
    public:
        SetObjectFolderCommand(std::vector<Scene::GameObject*> objects, std::string newFolder);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

    private:
        struct Entry
        {
            Scene::GameObject* object = nullptr;
            std::string oldFolder;
        };

        std::vector<Entry> m_entries;
        std::string m_newFolder;
    };
}
