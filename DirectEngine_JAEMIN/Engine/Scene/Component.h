#pragma once

namespace Engine::Scene
{
    class GameObject;

    class Component
    {
    public:
        virtual ~Component() = default;

        void SetOwner(GameObject* owner);
        GameObject* GetOwner() const;
        virtual void Update(float deltaTime);

    private:
        GameObject* m_owner = nullptr;
    };
}
