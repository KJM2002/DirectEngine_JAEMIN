#include "GameObject.h"

#include <algorithm>

namespace Engine::Scene
{
    GameObject::GameObject(std::string name)
        : m_id(GenerateObjectID())
        , m_name(std::move(name))
    {
    }

    GameObject::~GameObject() = default;

    ObjectID GameObject::GetID() const
    {
        return m_id;
    }

    void GameObject::SetID(ObjectID id)
    {
        if (id == InvalidObjectID)
        {
            m_id = GenerateObjectID();
            return;
        }

        m_id = id;
        ReserveObjectID(id);
    }

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

    ObjectID GameObject::GetParentID() const
    {
        return m_parentId;
    }

    void GameObject::SetParentID(ObjectID id)
    {
        m_parentId = id;
    }

    const std::vector<ObjectID>& GameObject::GetChildren() const
    {
        return m_children;
    }

    void GameObject::AddChildID(ObjectID id)
    {
        if (id == InvalidObjectID || std::find(m_children.begin(), m_children.end(), id) != m_children.end())
        {
            return;
        }

        m_children.push_back(id);
    }

    void GameObject::RemoveChildID(ObjectID id)
    {
        m_children.erase(std::remove(m_children.begin(), m_children.end(), id), m_children.end());
    }

    void GameObject::ClearChildren()
    {
        m_children.clear();
    }

    Math::Transform& GameObject::GetTransform()
    {
        return m_transform;
    }

    const Math::Transform& GameObject::GetTransform() const
    {
        return m_transform;
    }

    Component& GameObject::AddComponent(std::shared_ptr<Component> component)
    {
        if (!component)
        {
            component = std::make_shared<Component>();
        }

        component->SetOwner(this);
        Component& result = *component;
        m_components.push_back(std::move(component));
        return result;
    }

    Component* GameObject::GetComponentByID(ComponentID id) const
    {
        if (id == InvalidComponentID)
        {
            return nullptr;
        }

        for (const std::shared_ptr<Component>& component : m_components)
        {
            if (component && component->GetID() == id)
            {
                return component.get();
            }
        }

        return nullptr;
    }

    void GameObject::Update(float deltaTime)
    {
        for (const std::shared_ptr<Component>& component : m_components)
        {
            if (component && component->IsEnabled())
            {
                component->Update(deltaTime);
            }
        }
    }
}
