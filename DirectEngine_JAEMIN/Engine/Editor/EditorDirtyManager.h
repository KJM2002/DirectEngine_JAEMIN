#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Asset/AssetDatabase.h"

#include <string>

namespace Engine::Editor
{
    class EditorDirtyManager
    {
    public:
        void SetCurrentScene(Scene::Scene* scene);
        void SetAssetDatabase(Asset::AssetDatabase* assetDatabase);

        bool HasUnsavedSceneChanges() const;
        bool HasUnsavedAssetChanges() const;
        bool HasAnyUnsavedChanges() const;

        void MarkSceneDirty();
        void ClearSceneDirty();
        void MarkAssetDatabaseDirty();
        void ClearAssetDatabaseDirty();

        std::string GetSceneDisplayName() const;
        std::wstring BuildWindowTitle(const std::wstring& baseTitle) const;

    private:
        Scene::Scene* m_scene = nullptr;
        Asset::AssetDatabase* m_assetDatabase = nullptr;
    };
}
