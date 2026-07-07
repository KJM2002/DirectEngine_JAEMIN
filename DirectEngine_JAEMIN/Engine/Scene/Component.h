#pragma once

#include "SceneIDs.h"

namespace Engine::Scene
{
    class GameObject;

    class Component
    {
    public:
        Component();
        virtual ~Component() = default;

        ComponentID GetID() const;
        void SetID(ComponentID id);
        void SetOwner(GameObject* owner);
        GameObject* GetOwner() const;
        bool IsEnabled() const;
        void SetEnabled(bool enabled);
        virtual const char* GetTypeName() const;
        virtual void Update(float deltaTime);

    private:
        ComponentID m_id = InvalidComponentID;
        GameObject* m_owner = nullptr;
        bool m_enabled = true;
    };
}
