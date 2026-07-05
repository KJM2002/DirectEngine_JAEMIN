#include "GameObject.h"

namespace Engine::Scene
{
    GameObject::GameObject(std::string name)
        : m_name(std::move(name))
    {
    }

    GameObject::~GameObject() = default;

    const std::string& GameObject::GetName() const
    {
        return m_name;
    }

    void GameObject::SetName(std::string name)
    {
        m_name = std::move(name);
    }

    const std::string& GameObject::GetOutlinerFolder() const
    {
        return m_outlinerFolder;
    }

    void GameObject::SetOutlinerFolder(std::string folder)
    {
        m_outlinerFolder = std::move(folder);
    }

    Math::Transform& GameObject::GetTransform()
    {
        return m_transform;
    }

    const Math::Transform& GameObject::GetTransform() const
    {
        return m_transform;
    }

    void GameObject::Update(float deltaTime)
    {
        for (const std::shared_ptr<Component>& component : m_components)
        {
            component->Update(deltaTime);
        }
    }
}
