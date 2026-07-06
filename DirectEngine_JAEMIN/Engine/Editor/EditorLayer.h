#pragma once

#include "Engine/Asset/AssetDatabase.h"
#include "Engine/Editor/Command/GameObjectSnapshot.h"
#include "Engine/Editor/EditorDirtyManager.h"
#include "Engine/Editor/MeshEditor/StaticMeshEditor.h"
#include "Engine/Math/MathTypes.h"
#include "Engine/Math/Transform.h"
#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Physics/Ray.h"
#include "Engine/Physics/RaycastHit.h"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Engine::Debug
{
    struct DebugStats;
}

namespace Engine::Input
{
    class Input;
}

namespace Engine::Scene
{
    class Camera;
    class GameObject;
    class Scene;
}

namespace Engine::Renderer
{
    enum class GizmoVisualMode;
    class Material;
    class Renderer;
}

namespace Engine::Editor
{
    class CommandManager;

    enum class GizmoMode
    {
        Move,
        Rotate,
        Scale
    };

    enum class GizmoAxis
    {
        None,
        X,
        Y,
        Z
    };

    enum class MaterialTextureSlot
    {
        BaseColor,
        Roughness,
        Metallic,
        Normal
    };

    enum class SavePromptResult
    {
        None,
        Save,
        DontSave,
        Cancel
    };

    enum class PendingEditorAction
    {
        None,
        NewScene,
        OpenScene,
        Exit
    };

    class EditorLayer
    {
    public:
        EditorLayer();
        ~EditorLayer();

        bool Initialize();
        void Shutdown();
        void Update(Scene::Scene& scene, const Debug::DebugStats& stats, Input::Input& input, float deltaTime);
        void Render(Scene::Scene& scene, const Debug::DebugStats& stats, Renderer::Renderer& renderer);
        void ApplyRendererSettings(Renderer::Renderer& renderer, const Scene::Scene& scene) const;

        Scene::GameObject* GetSelectedObject() const;
        const std::vector<Scene::GameObject*>& GetSelectedObjects() const;
        bool IsPlayMode() const;
        bool WantsInputCapture() const;
        bool ShouldShowColliders() const;
        bool ShouldShowGizmo() const;
        Renderer::GizmoVisualMode GetGizmoVisualMode() const;
        void RequestExit(Scene::Scene& scene);
        bool ConsumeExitRequested();
        std::wstring BuildWindowTitle(const std::wstring& baseTitle, Scene::Scene& scene);

    private:
        void RefreshAssets();
        void EnsureCommandManager(Scene::Scene& scene);
        void RenderAssetBrowser(Scene::Scene& scene, Renderer::Renderer& renderer);
        void RenderObjectTools(Scene::Scene& scene, Renderer::Renderer& renderer);
        void ValidateSelection(const Scene::Scene& scene);
        bool RaycastSceneMeshes(const Scene::Scene& scene, const Physics::Ray& ray, Physics::RaycastHit& outHit) const;
        void SetSingleSelectedObject(Scene::GameObject* object);
        void ToggleSelectedObject(Scene::GameObject* object);
        bool IsObjectSelected(const Scene::GameObject* object) const;
        void SetSingleSelectedAsset(const std::wstring& asset);
        void ToggleSelectedAsset(const std::wstring& asset);
        bool IsAssetSelected(const std::wstring& asset) const;
        bool ApplyMaterialToSelectedObjects(Renderer::Renderer& renderer, const std::wstring& materialPath);
        bool DeleteSelectedObjects(Scene::Scene& scene);
        Scene::GameObject* DuplicateSelectedObject(Scene::Scene& scene);
        void CreateOutlinerFolder(Scene::Scene& scene);
        void BeginRenameSelectedFolder();
        void CommitFolderRename(Scene::Scene& scene);
        void MoveObjectsToFolder(Scene::Scene& scene, const std::vector<Scene::GameObject*>& objects, const std::string& folder);
        std::string MakeUniqueOutlinerFolderName(const Scene::Scene& scene, const std::string& baseName) const;
        void BeginRenameSelected();
        void CommitRename();
        void BeginRenameSelectedAsset();
        void CommitAssetRename(Scene::Scene& scene, Renderer::Renderer& renderer);
        bool CreateMaterialAsset(Renderer::Renderer& renderer);
        bool CreateSceneAsset(Scene::Scene& scene);
        bool RequestNewScene(Scene::Scene& scene);
        bool RequestOpenScene(Scene::Scene& scene, Renderer::Renderer& renderer, const std::wstring& path);
        bool CheckSaveBeforeDestructiveAction(Scene::Scene& scene, PendingEditorAction action, const std::wstring& path = {});
        void ExecutePendingAction(Scene::Scene& scene, Renderer::Renderer& renderer, SavePromptResult result);
        void ExecuteNewScene(Scene::Scene& scene);
        bool ExecuteOpenScene(Scene::Scene& scene, Renderer::Renderer& renderer, const std::wstring& path);
        void RenderSavePromptDialog(Scene::Scene& scene, Renderer::Renderer& renderer);
        bool SaveCurrentScene(Scene::Scene& scene);
        bool ImportModelAsset();
        bool SaveCurrentSceneAs(Scene::Scene& scene, const std::wstring& path);
        int SaveReferencedMaterialAssets(const Scene::Scene& scene);
        bool SaveSelectedMaterialAs(const std::wstring& path);
        bool DeleteSelectedMaterialAsset(Scene::Scene& scene, Renderer::Renderer& renderer);
        bool DeleteSelectedAssets(Scene::Scene& scene, Renderer::Renderer& renderer);
        bool ApplyTextureToSelectedMaterial(Renderer::Renderer& renderer, const std::wstring& texturePath);
        bool ApplyTextureToSelectedMaterial(Renderer::Renderer& renderer, const std::wstring& texturePath, MaterialTextureSlot slot);
        bool ApplyTextureToMaterial(Renderer::Renderer& renderer, Renderer::Material& material, const std::wstring& texturePath, MaterialTextureSlot slot);
        bool ClearSelectedMaterialTextureSlot(MaterialTextureSlot slot);
        bool ClearMaterialTextureSlot(Renderer::Material& material, MaterialTextureSlot slot);
        void OpenMaterialEditor(Renderer::Renderer& renderer, const std::wstring& materialAsset);
        void RenderMaterialEditor(Renderer::Renderer& renderer);
        bool SaveMaterialEditor();
        void OpenModelEditor(const std::wstring& modelAsset);
        void RenderModelEditor(Scene::Scene& scene, Renderer::Renderer& renderer);
        bool LoadModelColliderAsset(const std::wstring& modelAsset);
        bool SaveModelColliderAsset() const;
        bool ApplyModelCollidersToSelectedObjects();
        GizmoAxis PickGizmoAxis(const Scene::Camera& camera, int mouseX, int mouseY, float width, float height) const;
        void BeginGizmoDrag(const Scene::Camera& camera, const Input::Input& input, float width, float height);
        void EndGizmoDrag();
        void ApplyGizmoDrag(const Scene::Camera& camera, const Input::Input& input, float width, float height);
        void ShowNotification(const std::string& text, float seconds = 2.0f);

        Scene::GameObject* m_selectedObject = nullptr;
        std::vector<Scene::GameObject*> m_selectedObjects;
        std::unique_ptr<CommandManager> m_commandManager;
        GizmoMode m_gizmoMode = GizmoMode::Move;
        GizmoAxis m_activeGizmoAxis = GizmoAxis::None;
        float m_moveSnapRemainder = 0.0f;
        float m_rotateSnapRemainder = 0.0f;
        float m_scaleSnapRemainder = 0.0f;
        float m_moveSnapStep = 0.1f;
        float m_rotateSnapStep = 0.1f;
        float m_scaleSnapStep = 0.1f;
        std::array<Math::Vector3, 3> m_dragStartLocalAxes = {};
        std::vector<std::pair<Scene::GameObject*, Math::Transform>> m_gizmoDragStartTransforms;
        Math::Vector3 m_gizmoDragCenter = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 m_gizmoDragAxis = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 m_gizmoDragStartVector = { 0.0f, 0.0f, 0.0f };
        float m_gizmoDragScreenCenterX = 0.0f;
        float m_gizmoDragScreenCenterY = 0.0f;
        float m_gizmoDragStartScreenAngle = 0.0f;
        bool m_gizmoRotateDragValid = false;
        bool m_debugRotationGizmo = false;
        Math::Transform m_gizmoDragStartTransform;
        Scene::GameObject* m_gizmoDragObject = nullptr;
        Math::Transform m_inspectorTransformStart;
        Scene::GameObject* m_inspectorTransformObject = nullptr;
        GameObjectSnapshot m_inspectorEditStart;
        Scene::GameObject* m_inspectorEditObject = nullptr;
        std::array<char, 128> m_renameBuffer = {};
        std::array<char, 128> m_newMaterialNameBuffer = {};
        std::array<char, 128> m_newSceneNameBuffer = {};
        std::array<char, 128> m_saveMaterialAsBuffer = {};
        std::array<char, 128> m_assetRenameBuffer = {};
        std::array<char, 128> m_folderRenameBuffer = {};
        Scene::GameObject* m_renamingObject = nullptr;
        std::string m_selectedOutlinerFolder;
        std::string m_renamingOutlinerFolder;
        Scene::GameObject* m_outlinerSelectionAnchor = nullptr;
        bool m_focusRenameInput = false;
        bool m_focusAssetRenameInput = false;
        bool m_focusFolderRenameInput = false;
        bool m_playMode = false;
        bool m_wantsInputCapture = false;
        bool m_showColliders = true;
        bool m_showAssetBrowser = true;
        bool m_showGizmoTools = true;
        float m_toolbarHeight = 78.0f;
        float m_rightPanelWidth = 430.0f;
        float m_bottomPanelHeight = 280.0f;
        float m_hierarchyPanelRatio = 0.36f;
        std::string m_notificationText;
        float m_notificationTimer = 0.0f;
        Asset::AssetDatabase m_assetDatabase;
        EditorDirtyManager m_dirtyManager;
        StaticMeshEditor m_staticMeshEditor;
        PendingEditorAction m_pendingAction = PendingEditorAction::None;
        std::wstring m_pendingOpenScenePath;
        bool m_openSavePrompt = false;
        bool m_exitRequested = false;
        struct ModelColliderEntry
        {
            Physics::ColliderType type = Physics::ColliderType::AABB;
            Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 size = { 1.0f, 1.0f, 1.0f };
            float radius = 0.5f;
            bool isTrigger = false;
        };
        bool m_showModelEditor = false;
        std::wstring m_modelEditorAsset;
        std::vector<ModelColliderEntry> m_modelEditorColliders;
        int m_selectedModelColliderIndex = -1;
        bool m_showMaterialEditor = false;
        std::wstring m_materialEditorAsset;
        std::shared_ptr<Renderer::Material> m_materialEditorMaterial;
        bool m_materialEditorDirty = false;
        std::wstring m_selectedAssetFolder = L"Assets";
        std::wstring m_currentSceneAsset = L"Assets/Scenes/Test.scene";
        std::wstring m_selectedAsset;
        std::vector<std::wstring> m_selectedAssets;
        std::wstring m_renamingAsset;
        std::wstring m_selectedSceneAsset;
        std::wstring m_selectedMaterialAsset;
        std::vector<std::wstring> m_modelAssets;
        std::vector<std::wstring> m_textureAssets;
        std::vector<std::wstring> m_materialAssets;
        std::vector<std::wstring> m_sceneAssets;
    };
}
