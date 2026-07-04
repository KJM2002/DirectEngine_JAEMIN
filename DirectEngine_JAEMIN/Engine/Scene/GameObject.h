#pragma once

#include "Component.h"
#include "Engine/Math/Transform.h"

#include <memory>
#include <string>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

namespace Engine::Scene
{
    class GameObject
    {
    public:
        explicit GameObject(std::string name);
        ~GameObject();

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;

        const std::string& GetName() const;
        void SetName(std::string name);
        Math::Transform& GetTransform();
        const Math::Transform& GetTransform() const;

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

            auto component = std::make_shared<T>(std::forward<Args>(args)...);
            component->SetOwner(this);
            T& result = *component;
            m_components.push_back(std::move(component));
            return result;
        }

        template <typename T>
        T* GetComponent() const
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

            for (const std::shared_ptr<Component>& component : m_components)
            {
                if (T* result = dynamic_cast<T*>(component.get()))
                {
                    return result;
                }
            }

            return nullptr;
        }

        template <typename T>
        std::vector<T*> GetComponents() const
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

            std::vector<T*> results;
            for (const std::shared_ptr<Component>& component : m_components)
            {
                if (T* result = dynamic_cast<T*>(component.get()))
                {
                    results.push_back(result);
                }
            }

            return results;
        }

        template <typename T>
        void RemoveComponents()
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

            m_components.erase(std::remove_if(m_components.begin(), m_components.end(),
                [](const std::shared_ptr<Component>& component)
                {
                    return dynamic_cast<T*>(component.get()) != nullptr;
                }),
                m_components.end());
        }

        void Update(float deltaTime);

    private:
        std::string m_name;
        Math::Transform m_transform;
        std::vector<std::shared_ptr<Component>> m_components;
    };
}
