#include "SetObjectFolderCommand.h"

#include "Engine/Scene/GameObject.h"

namespace Engine::Editor
{
    SetObjectFolderCommand::SetObjectFolderCommand(std::vector<Scene::GameObject*> objects, std::string newFolder)
        : m_newFolder(std::move(newFolder))
    {
        m_entries.reserve(objects.size());
        for (Scene::GameObject* object : objects)
        {
            if (!object)
            {
                continue;
            }

            m_entries.push_back({ object, object->GetOutlinerFolder() });
        }
    }

    void SetObjectFolderCommand::Execute()
    {
        for (Entry& entry : m_entries)
        {
            if (entry.object)
            {
                entry.object->SetOutlinerFolder(m_newFolder);
            }
        }
    }

    void SetObjectFolderCommand::Undo()
    {
        for (Entry& entry : m_entries)
        {
            if (entry.object)
            {
                entry.object->SetOutlinerFolder(entry.oldFolder);
            }
        }
    }

    const char* SetObjectFolderCommand::GetName() const
    {
        return "Move To Folder";
    }
}
