#include "Component.h"

namespace Engine::Scene
{
    Component::Component()
        : m_id(GenerateComponentID())
    {
    }

    ComponentID Component::GetID() const
    {
        return m_id;
    }

    void Component::SetID(ComponentID id)
    {
        if (id == InvalidComponentID)
        {
            m_id = GenerateComponentID();
            return;
        }

        m_id = id;
        ReserveComponentID(id);
    }

    void Component::SetOwner(GameObject* owner)
    {
        m_owner = owner;
    }

    GameObject* Component::GetOwner() const
    {
        return m_owner;
    }

    bool Component::IsEnabled() const
    {
        return m_enabled;
    }

    void Component::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    const char* Component::GetTypeName() const
    {
        return "Component";
    }

    void Component::Update(float)
    {
    }
}
