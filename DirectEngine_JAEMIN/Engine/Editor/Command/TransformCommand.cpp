#include "TransformCommand.h"

#include "Engine/Scene/GameObject.h"

#include <cmath>

namespace Engine::Editor
{
    namespace
    {
        bool Different(float a, float b, float epsilon)
        {
            return std::abs(a - b) > epsilon;
        }

        bool Different(const Math::Vector3& a, const Math::Vector3& b, float epsilon)
        {
            return Different(a.x, b.x, epsilon)
                || Different(a.y, b.y, epsilon)
                || Different(a.z, b.z, epsilon);
        }
    }

    TransformCommand::TransformCommand(Scene::GameObject& object, const Math::Transform& before, const Math::Transform& after)
        : m_object(object)
        , m_before(before)
        , m_after(after)
    {
    }

    void TransformCommand::Execute()
    {
        m_object.GetTransform() = m_after;
    }

    void TransformCommand::Undo()
    {
        m_object.GetTransform() = m_before;
    }

    const char* TransformCommand::GetName() const
    {
        return "Transform Object";
    }

    bool TransformCommand::IsDifferent(const Math::Transform& a, const Math::Transform& b, float epsilon)
    {
        return Different(a.position, b.position, epsilon)
            || Different(a.rotation, b.rotation, epsilon)
            || Different(a.scale, b.scale, epsilon);
    }
}
