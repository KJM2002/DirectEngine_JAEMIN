#pragma once

#include "ICommand.h"
#include "Engine/Math/Transform.h"

namespace Engine::Scene
{
    class GameObject;
}

namespace Engine::Editor
{
    class TransformCommand : public ICommand
    {
    public:
        TransformCommand(Scene::GameObject& object, const Math::Transform& before, const Math::Transform& after);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

        static bool IsDifferent(const Math::Transform& a, const Math::Transform& b, float epsilon = 0.0001f);

    private:
        Scene::GameObject& m_object;
        Math::Transform m_before;
        Math::Transform m_after;
    };
}
