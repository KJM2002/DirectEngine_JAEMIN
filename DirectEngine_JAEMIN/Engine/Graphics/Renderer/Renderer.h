#pragma once

#include "Engine/Graphics/Renderer/PostProcess.h"
#include "Engine/Graphics/RHI/RHICommon.h"
#include "Engine/Math/Transform.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIDevice;
    class RHISwapChain;
    class RHITexture;
}

namespace Engine::Scene
{
    class Camera;
    class DirectionalLight;
    class GameObject;
    class Scene;
}

namespace Engine::Resource
{
    class ResourceManager;
}

namespace Engine::Renderer
{
    class Material;
    class Mesh;
    class DebugRenderer;
    class GizmoRenderer;
    class SelectionOutlineRenderer;
    class ShadowMap;

    enum class GizmoVisualMode
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

    struct RenderLight
    {
        Math::Vector3 position = { 0.0f, 0.0f, 0.0f };
        float range = 0.0f;
        Math::Vector3 direction = { 0.0f, -1.0f, 0.0f };
        float type = 0.0f;
        Math::Vector3 color = { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        Math::Vector3 spotAngles = { 1.0f, 0.0f, 0.0f };
        float padding = 0.0f;
    };

    struct SelectionOutlineSettings
    {
        bool enabled = true;
        bool debugLineEnabled = false;
        Math::Vector4 color = { 1.0f, 0.78f, 0.08f, 1.0f };
        float width = 4.0f;
        float opacity = 1.0f;
    };

    struct RenderItem
    {
        const Mesh* mesh = nullptr;
        const Material* material = nullptr;
        Math::Transform transform;
    };

    class RenderQueue
    {
    public:
        void Clear();
        void Submit(RenderItem item);
        const std::vector<RenderItem>& GetItems() const;

    private:
        std::vector<RenderItem> m_items;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        bool Initialize(RHI::GraphicsAPI api, void* nativeWindowHandle, std::uint32_t width, std::uint32_t height);
        void Shutdown();
        void Update(float deltaTime);
        void BeginFrame(float red, float green, float blue, float alpha, bool selectionOutlineCandidate = false);
        void EndFrame();
        void Resize(std::uint32_t width, std::uint32_t height);
        void SetActiveCamera(const Scene::Camera* camera);
        void SetDirectionalLight(const Scene::DirectionalLight* light);
        void SetLights(const std::vector<RenderLight>& lights, const Math::Vector3& ambientColor);
        Resource::ResourceManager& GetResourceManager();
        std::shared_ptr<Mesh> CreateCubeMesh();
        std::shared_ptr<Material> CreateMaterial(const Math::Vector4& baseColor);
        void DrawMesh(const Mesh& mesh, const Math::Transform& transform);
        void DrawMesh(const Mesh& mesh, const Material& material, const Math::Transform& transform);
        void SubmitMesh(const Mesh& mesh, const Material& material, const Math::Transform& transform);
        void FlushRenderQueue();
        void RenderShadowPass(const Scene::Scene& scene);
        void Render(const Scene::GameObject& object);
        void RenderColliders(const Scene::Scene& scene, const Scene::GameObject* selectedObject, bool enabled);
        void RenderSelectionOutlines(const std::vector<Scene::GameObject*>& selectedObjects);
        void RenderSelectionGizmo(const Scene::GameObject* selectedObject, bool enabled, GizmoVisualMode mode, GizmoAxis hoveredAxis = GizmoAxis::None, GizmoAxis activeAxis = GizmoAxis::None);
        void* RenderStaticMeshPreview(const Mesh& mesh, std::uint32_t width, std::uint32_t height, const Math::Matrix4x4& view, const Math::Matrix4x4& projection);
        void ApplyPostProcess();
        void SetPostProcessSettings(const PostProcessSettings& settings);
        PostProcessSettings GetPostProcessSettings() const;
        void SetSelectionOutlineSettings(const SelectionOutlineSettings& settings);
        SelectionOutlineSettings GetSelectionOutlineSettings() const;
        std::uint32_t GetDrawCallCount() const;
        std::uint32_t GetTriangleCount() const;
        void* GetNativeDeviceHandleForDebugUI() const;
        void* GetNativeContextHandleForDebugUI() const;

    private:
        bool CreateRenderResources();
        void DrawMeshWithMatrices(const Mesh& mesh, const Material& material, const Math::Transform& transform, const Math::Matrix4x4& view, const Math::Matrix4x4& projection);
        void RenderDebugSelectionOutlines(const std::vector<Scene::GameObject*>& selectedObjects);

        std::unique_ptr<RHI::RHIDevice> m_device;
        std::unique_ptr<RHI::RHISwapChain> m_swapChain;
        std::unique_ptr<Resource::ResourceManager> m_resourceManager;
        std::unique_ptr<DebugRenderer> m_debugRenderer;
        std::unique_ptr<GizmoRenderer> m_gizmoRenderer;
        std::unique_ptr<SelectionOutlineRenderer> m_selectionOutlineRenderer;
        std::unique_ptr<PostProcessStack> m_postProcessStack;
        std::unique_ptr<ShadowMap> m_shadowMap;
        std::shared_ptr<Material> m_defaultMaterial;
        std::shared_ptr<RHI::RHIBuffer> m_transformConstantBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_materialConstantBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_lightConstantBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_shadowConstantBuffer;
        std::shared_ptr<RHI::RHITexture> m_sceneColorTarget;
        std::shared_ptr<RHI::RHITexture> m_sceneDepthTarget;
        std::shared_ptr<RHI::RHITexture> m_previewColorTarget;
        std::shared_ptr<RHI::RHITexture> m_previewDepthTarget;
        const Scene::Camera* m_activeCamera = nullptr;
        const Scene::DirectionalLight* m_directionalLight = nullptr;
        std::vector<RenderLight> m_lights;
        Math::Vector3 m_ambientColor = { 0.18f, 0.20f, 0.24f };
        RenderQueue m_renderQueue;
        SelectionOutlineSettings m_selectionOutlineSettings;
        std::uint32_t m_drawCallCount = 0;
        std::uint32_t m_triangleCount = 0;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::uint32_t m_previewWidth = 0;
        std::uint32_t m_previewHeight = 0;
        bool m_enableShadows = false;
        bool m_enablePostProcess = false;
        bool m_frameUsesSceneColorTarget = false;
        bool m_selectionOutlineMaskReady = false;
    };
}
