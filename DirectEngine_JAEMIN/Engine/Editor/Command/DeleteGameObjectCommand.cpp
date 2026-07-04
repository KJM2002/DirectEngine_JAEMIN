#include "DeleteGameObjectCommand.h"

#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Scene.h"

#include <algorithm>

namespace Engine::Editor
{
    DeleteGameObjectCommand::DeleteGameObjectCommand(Scene::Scene& scene, const std::vector<Scene::GameObject*>& objects)
        : m_scene(scene)
        , m_objects(objects)
    {
        for (const Scene::GameObject* object : objects)
        {
            if (object)
            {
                m_snapshots.push_back(GameObjectSnapshot::Capture(*object));
            }
        }
    }

    void DeleteGameObjectCommand::Execute()
    {
        for (Scene::GameObject* object : m_objects)
        {
            m_scene.DestroyGameObject(object);
        }
        m_objects.clear();
    }

    void DeleteGameObjectCommand::Undo()
    {
        m_objects.clear();
        for (const GameObjectSnapshot& snapshot : m_snapshots)
        {
            m_objects.push_back(&snapshot.Restore(m_scene));
        }
    }

    const char* DeleteGameObjectCommand::GetName() const
    {
        return "Delete Object";
    }
}
