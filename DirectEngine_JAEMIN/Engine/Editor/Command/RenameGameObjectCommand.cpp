#include "RenameGameObjectCommand.h"

#include "Engine/Scene/GameObject.h"

namespace Engine::Editor
{
    RenameGameObjectCommand::RenameGameObjectCommand(Scene::GameObject& object, std::string oldName, std::string newName)
        : m_object(object)
        , m_oldName(std::move(oldName))
        , m_newName(std::move(newName))
    {
    }

    void RenameGameObjectCommand::Execute()
    {
        m_object.SetName(m_newName);
    }

    void RenameGameObjectCommand::Undo()
    {
        m_object.SetName(m_oldName);
    }

    const char* RenameGameObjectCommand::GetName() const
    {
        return "Rename Object";
    }
}
