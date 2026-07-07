#include "DuplicateGameObjectCommand.h"

#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Scene.h"

#include <algorithm>

namespace Engine::Editor
{
    DuplicateGameObjectCommand::DuplicateGameObjectCommand(Scene::Scene& scene, const std::vector<Scene::GameObject*>& sources)
        : m_scene(scene)
    {
        for (const Scene::GameObject* source : sources)
        {
            if (!source)
            {
                continue;
            }

            GameObjectSnapshot snapshot = GameObjectSnapshot::Capture(*source);
            snapshot.name = MakeUniqueName(source->GetName());
            snapshot.ClearPersistentIDs();
            snapshot.meshRenderer.cloneMaterialOnRestore = true;
            m_snapshots.push_back(std::move(snapshot));
        }
    }

    void DuplicateGameObjectCommand::Execute()
    {
        m_duplicates.clear();
        for (const GameObjectSnapshot& snapshot : m_snapshots)
        {
            m_duplicates.push_back(&snapshot.Restore(m_scene));
        }
    }

    void DuplicateGameObjectCommand::Undo()
    {
        for (Scene::GameObject* object : m_duplicates)
        {
            m_scene.DestroyGameObject(object);
        }
        m_duplicates.clear();
    }

    const char* DuplicateGameObjectCommand::GetName() const
    {
        return "Duplicate Object";
    }

    const std::vector<Scene::GameObject*>& DuplicateGameObjectCommand::GetDuplicates() const
    {
        return m_duplicates;
    }

    std::string DuplicateGameObjectCommand::MakeUniqueName(const std::string& sourceName) const
    {
        std::string baseName = sourceName;
        while (!baseName.empty() && std::isdigit(static_cast<unsigned char>(baseName.back())))
        {
            baseName.pop_back();
        }
        if (baseName.empty())
        {
            baseName = sourceName;
        }

        int highestSuffix = 0;
        for (const std::unique_ptr<Scene::GameObject>& object : m_scene.GetGameObjects())
        {
            const std::string& name = object->GetName();
            if (name == baseName)
            {
                highestSuffix = std::max(highestSuffix, 1);
                continue;
            }
            if (name.rfind(baseName, 0) == 0)
            {
                const std::string suffixText = name.substr(baseName.size());
                if (!suffixText.empty() && std::all_of(suffixText.begin(), suffixText.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
                {
                    highestSuffix = std::max(highestSuffix, std::stoi(suffixText));
                }
            }
        }

        return baseName + std::to_string(highestSuffix + 1);
    }
}
