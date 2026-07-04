#include "Component.h"

namespace Engine::Scene
{
    void Component::SetOwner(GameObject* owner)
    {
        m_owner = owner;
    }

    GameObject* Component::GetOwner() const
    {
        return m_owner;
    }

    void Component::Update(float)
    {
    }
}
