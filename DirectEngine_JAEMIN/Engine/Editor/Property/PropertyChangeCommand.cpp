#include "PropertyChangeCommand.h"

#include "Engine/Editor/Property/PropertySystem.h"

#include <utility>

namespace Engine::Editor
{
    PropertyChangeCommand::PropertyChangeCommand(Scene::Scene& scene,
        Scene::ObjectID objectId,
        Scene::ComponentID componentId,
        std::string propertyPath,
        PropertyValue oldValue,
        PropertyValue newValue)
        : m_scene(scene)
        , m_objectId(objectId)
        , m_componentId(componentId)
        , m_propertyPath(std::move(propertyPath))
        , m_oldValue(std::move(oldValue))
        , m_newValue(std::move(newValue))
    {
    }

    void PropertyChangeCommand::Execute()
    {
        PropertySystem::ApplyProperty(m_scene, m_objectId, m_componentId, m_propertyPath, m_newValue);
    }

    void PropertyChangeCommand::Undo()
    {
        PropertySystem::ApplyProperty(m_scene, m_objectId, m_componentId, m_propertyPath, m_oldValue);
    }

    const char* PropertyChangeCommand::GetName() const
    {
        return "Change Property";
    }
}
