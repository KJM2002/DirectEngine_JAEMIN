#include "CreateGameObjectCommand.h"

#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Scene.h"

namespace Engine::Editor
{
    CreateGameObjectCommand::CreateGameObjectCommand(Scene::Scene& scene, GameObjectSnapshot snapshot)
        : m_scene(scene)
        , m_snapshot(std::move(snapshot))
    {
    }

    void CreateGameObjectCommand::Execute()
    {
        m_object = &m_snapshot.Restore(m_scene);
    }

    void CreateGameObjectCommand::Undo()
    {
        if (m_object)
        {
            m_scene.DestroyGameObject(m_object);
            m_object = nullptr;
        }
    }

    const char* CreateGameObjectCommand::GetName() const
    {
        return "Create Object";
    }

    Scene::GameObject* CreateGameObjectCommand::GetCreatedObject() const
    {
        return m_object;
    }
}
