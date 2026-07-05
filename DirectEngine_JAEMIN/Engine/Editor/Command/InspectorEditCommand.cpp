#include "InspectorEditCommand.h"

#include "Engine/Scene/GameObject.h"

namespace Engine::Editor
{
    InspectorEditCommand::InspectorEditCommand(Scene::GameObject& object, const GameObjectSnapshot& before, const GameObjectSnapshot& after)
        : m_object(object)
        , m_before(before)
        , m_after(after)
    {
    }

    void InspectorEditCommand::Execute()
    {
        m_after.ApplyTo(m_object);
    }

    void InspectorEditCommand::Undo()
    {
        m_before.ApplyTo(m_object);
    }

    const char* InspectorEditCommand::GetName() const
    {
        return "Edit Inspector";
    }
}
