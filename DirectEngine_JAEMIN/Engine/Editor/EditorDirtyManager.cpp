#include "EditorDirtyManager.h"

namespace Engine::Editor
{
    namespace
    {
        std::wstring ToWideAscii(const std::string& value)
        {
            std::wstring result;
            result.reserve(value.size());
            for (char c : value)
            {
                result.push_back(static_cast<unsigned char>(c));
            }
            return result;
        }
    }

    void EditorDirtyManager::SetCurrentScene(Scene::Scene* scene)
    {
        m_scene = scene;
    }

    void EditorDirtyManager::SetAssetDatabase(Asset::AssetDatabase* assetDatabase)
    {
        m_assetDatabase = assetDatabase;
    }

    bool EditorDirtyManager::HasUnsavedSceneChanges() const
    {
        return m_scene && m_scene->IsDirty();
    }

    bool EditorDirtyManager::HasUnsavedAssetChanges() const
    {
        return m_assetDatabase && m_assetDatabase->IsDirty();
    }

    bool EditorDirtyManager::HasAnyUnsavedChanges() const
    {
        return HasUnsavedSceneChanges() || HasUnsavedAssetChanges();
    }

    void EditorDirtyManager::MarkSceneDirty()
    {
        if (m_scene)
        {
            m_scene->MarkDirty();
        }
    }

    void EditorDirtyManager::ClearSceneDirty()
    {
        if (m_scene)
        {
            m_scene->ClearDirty();
        }
    }

    void EditorDirtyManager::MarkAssetDatabaseDirty()
    {
        if (m_assetDatabase)
        {
            m_assetDatabase->MarkDirty();
        }
    }

    void EditorDirtyManager::ClearAssetDatabaseDirty()
    {
        if (m_assetDatabase)
        {
            m_assetDatabase->ClearDirty();
        }
    }

    std::string EditorDirtyManager::GetSceneDisplayName() const
    {
        return m_scene ? m_scene->GetDirtyDisplayName() : "Untitled.scene";
    }

    std::wstring EditorDirtyManager::BuildWindowTitle(const std::wstring& baseTitle) const
    {
        std::wstring title = baseTitle;
        title += L" | Scene ";
        title += ToWideAscii(GetSceneDisplayName());
        if (HasUnsavedAssetChanges())
        {
            title += L" | Assets*";
        }
        return title;
    }
}
