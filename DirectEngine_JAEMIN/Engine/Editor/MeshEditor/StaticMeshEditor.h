#pragma once

#include "Engine/Editor/MeshEditor/StaticMeshPreviewViewport.h"

#include <filesystem>
#include <memory>
#include <string>

namespace Engine::Asset
{
    class AssetDatabase;
}

namespace Engine::Resource
{
    class ResourceManager;
}

namespace Engine::Renderer
{
    class Renderer;
}

namespace Engine::Editor
{
    class StaticMeshEditor
    {
    public:
        void OpenEmpty();
        bool OpenByGuid(const std::string& meshGuid, const Asset::AssetDatabase& assetDatabase, Resource::ResourceManager& resourceManager);
        bool OpenByPath(const std::filesystem::path& meshPath, const Asset::AssetDatabase& assetDatabase, Resource::ResourceManager& resourceManager);
        void Close();
        bool IsOpen() const;
        void OnUpdate(float deltaTime);
        void OnImGuiRender(Renderer::Renderer& renderer);

    private:
        void DrawToolbar();
        void DrawDetailsPanel();
        void DrawMaterialSlotsPanel();
        void DrawColliderPanel();
        bool LoadColliderAsset();
        bool SaveColliderAsset() const;
        void SyncCollidersToPreview();
        void SetError(const std::string& error);

        bool m_isOpen = false;
        std::string m_meshGuid;
        std::filesystem::path m_meshPath;
        std::shared_ptr<Renderer::Mesh> m_mesh;
        StaticMeshPreviewViewport m_previewViewport;
        std::vector<StaticMeshPreviewViewport::ColliderPreview> m_colliders;
        int m_selectedColliderIndex = -1;
        bool m_showGrid = false;
        bool m_showAxis = true;
        bool m_showBounds = false;
        bool m_showCollider = false;
        ColliderGizmoMode m_colliderGizmoMode = ColliderGizmoMode::Move;
        std::string m_errorText;
    };
}
