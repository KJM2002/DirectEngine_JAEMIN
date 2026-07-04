#include "Renderer.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Graphics/RHI/RHIFactory.h"
#include "Engine/Graphics/RHI/RHISwapChain.h"
#include "Engine/Math/Transform.h"
#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Light.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/Scene.h"

#include "DebugRenderer.h"
#include "Material.h"
#include "Mesh.h"
#include "ShadowMap.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace Engine::Renderer
{
    namespace
    {
        Math::Vector3 Add(const Math::Vector3& a, const Math::Vector3& b)
        {
            return { a.x + b.x, a.y + b.y, a.z + b.z };
        }

        Math::Vector3 Scale(const Math::Vector3& value, float amount)
        {
            return { value.x * amount, value.y * amount, value.z * amount };
        }

        Math::Vector3 RotateAxis(const Math::Vector3& rotationValue, const Math::Vector3& axis)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(rotationValue.x, rotationValue.y, rotationValue.z);
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&axis), rotation)));
            return result;
        }
    }

    void RenderQueue::Clear()
    {
        m_items.clear();
    }

    void RenderQueue::Submit(RenderItem item)
    {
        if (item.mesh && item.material)
        {
            m_items.push_back(item);
        }
    }

    const std::vector<RenderItem>& RenderQueue::GetItems() const
    {
        return m_items;
    }

    namespace
    {
        constexpr std::uint32_t MaxRenderLights = 16;

        struct TransformBufferData
        {
            Math::Matrix4x4 world;
            Math::Matrix4x4 view;
            Math::Matrix4x4 projection;
        };

        struct MaterialBufferData
        {
            Math::Vector4 baseColor;
            std::int32_t useBaseColorTexture = 0;
            std::int32_t useRoughnessTexture = 0;
            std::int32_t useMetallicTexture = 0;
            std::int32_t useNormalTexture = 0;
            float roughness = 0.5f;
            float metallic = 0.0f;
            float padding0 = 0.0f;
            float padding1 = 0.0f;
        };

        struct LightBufferData
        {
            RenderLight lights[MaxRenderLights];
            Math::Vector4 ambientColor;
            std::uint32_t lightCount = 0;
            float padding[3] = {};
        };

        struct ShadowBufferData
        {
            Math::Matrix4x4 lightView;
            Math::Matrix4x4 lightProjection;
            Math::Vector4 shadowParams = { 0.0f, 0.0015f, 0.0f, 0.0f };
        };
    }

    Renderer::Renderer() = default;
    Renderer::~Renderer() = default;

    bool Renderer::Initialize(RHI::GraphicsAPI api, void* nativeWindowHandle, std::uint32_t width, std::uint32_t height)
    {
        m_width = width;
        m_height = height;

        m_device = RHI::CreateRHIDevice(api);
        if (!m_device)
        {
            Core::Log::Error("Failed to create RHI device.");
            return false;
        }

        if (!m_device->Initialize())
        {
            Core::Log::Error("Failed to initialize RHI device.");
            return false;
        }

        RHI::SwapChainDesc swapChainDesc;
        swapChainDesc.nativeWindowHandle = nativeWindowHandle;
        swapChainDesc.width = width;
        swapChainDesc.height = height;
        swapChainDesc.enableVSync = true;

        m_swapChain = m_device->CreateSwapChain(swapChainDesc);
        if (!m_swapChain)
        {
            Core::Log::Error("Failed to create RHI swap chain.");
            return false;
        }

        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        m_device->GetContext().SetViewport(viewport);
        m_device->GetContext().SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);

        m_resourceManager = std::make_unique<Resource::ResourceManager>(*m_device);

        if (!CreateRenderResources())
        {
            Core::Log::Error("Failed to create cube render resources.");
            return false;
        }

        m_debugRenderer = std::make_unique<DebugRenderer>();
        if (!m_debugRenderer->Initialize(*m_device))
        {
            Core::Log::Error("Failed to initialize debug renderer.");
            return false;
        }

        m_shadowMap = std::make_unique<ShadowMap>();
        if (!m_shadowMap->Initialize(*m_device))
        {
            Core::Log::Error("Failed to initialize shadow map.");
            return false;
        }

        m_postProcessStack = std::make_unique<PostProcessStack>();
        if (!m_postProcessStack->Initialize(*m_device))
        {
            Core::Log::Error("Failed to initialize post process stack.");
            return false;
        }

        return true;
    }

    void Renderer::Shutdown()
    {
        if (m_swapChain)
        {
            m_swapChain->Shutdown();
            m_swapChain.reset();
        }

        m_transformConstantBuffer.reset();
        m_materialConstantBuffer.reset();
        m_lightConstantBuffer.reset();
        m_shadowConstantBuffer.reset();
        m_sceneColorTarget.reset();
        m_sceneDepthTarget.reset();
        if (m_debugRenderer)
        {
            m_debugRenderer->Shutdown();
            m_debugRenderer.reset();
        }
        if (m_shadowMap)
        {
            m_shadowMap->Shutdown();
            m_shadowMap.reset();
        }
        if (m_postProcessStack)
        {
            m_postProcessStack->Shutdown();
            m_postProcessStack.reset();
        }
        m_defaultMaterial.reset();
        if (m_resourceManager)
        {
            m_resourceManager->Clear();
            m_resourceManager.reset();
        }

        if (m_device)
        {
            m_device->Shutdown();
            m_device.reset();
        }
    }

    void Renderer::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    void Renderer::BeginFrame(float red, float green, float blue, float alpha)
    {
        m_drawCallCount = 0;
        m_triangleCount = 0;
        m_renderQueue.Clear();

        if (!m_device)
        {
            return;
        }

        RHI::RHIContext& context = m_device->GetContext();
        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        if (m_enablePostProcess)
        {
            context.SetRenderTarget(m_sceneColorTarget, m_sceneDepthTarget);
        }
        else
        {
            context.SetDefaultRenderTarget();
        }

        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);
        context.ClearRenderTarget(red, green, blue, alpha);
        context.ClearDepthStencil(1.0f, 0);
    }

    void Renderer::EndFrame()
    {
        if (m_swapChain)
        {
            m_swapChain->Present();
        }
    }

    void Renderer::Resize(std::uint32_t width, std::uint32_t height)
    {
        m_width = width;
        m_height = height;

        if (m_swapChain)
        {
            m_swapChain->Resize(width, height);
        }

        if (m_device)
        {
            m_sceneColorTarget = m_device->CreateRenderTarget(width, height, RHI::TextureFormat::RGBA8);
            m_sceneDepthTarget = m_device->CreateDepthTarget(width, height, RHI::TextureFormat::D24S8, false);

            RHI::ViewportDesc viewport;
            viewport.width = static_cast<float>(width);
            viewport.height = static_cast<float>(height);
            m_device->GetContext().SetViewport(viewport);
        }
    }

    void Renderer::SetActiveCamera(const Scene::Camera* camera)
    {
        m_activeCamera = camera;
    }

    void Renderer::SetDirectionalLight(const Scene::DirectionalLight* light)
    {
        m_directionalLight = light;
    }

    void Renderer::SetLights(const std::vector<RenderLight>& lights, const Math::Vector3& ambientColor)
    {
        m_lights = lights;
        if (m_lights.size() > MaxRenderLights)
        {
            Core::Log::Warning("Scene has more lights than the renderer limit; extra lights will be ignored.");
        }
        m_ambientColor = ambientColor;
    }

    Resource::ResourceManager& Renderer::GetResourceManager()
    {
        return *m_resourceManager;
    }

    std::shared_ptr<Mesh> Renderer::CreateCubeMesh()
    {
        if (!m_resourceManager)
        {
            return nullptr;
        }

        return m_resourceManager->CreateCubeMesh();
    }

    std::shared_ptr<Material> Renderer::CreateMaterial(const Math::Vector4& baseColor)
    {
        if (!m_resourceManager)
        {
            return nullptr;
        }

        const std::string key = "runtime:material:"
            + std::to_string(baseColor.x) + ":"
            + std::to_string(baseColor.y) + ":"
            + std::to_string(baseColor.z) + ":"
            + std::to_string(baseColor.w);
        return m_resourceManager->CreateMaterial(key, baseColor);
    }

    void Renderer::DrawMesh(const Mesh& mesh, const Math::Transform& transform)
    {
        if (!m_defaultMaterial)
        {
            return;
        }

        DrawMesh(mesh, *m_defaultMaterial, transform);
    }

    void Renderer::DrawMesh(const Mesh& mesh, const Material& material, const Math::Transform& transform)
    {
        if (!m_device || !mesh.GetVertexBuffer() || !mesh.GetIndexBuffer() || !m_transformConstantBuffer || !m_materialConstantBuffer || !m_lightConstantBuffer || !m_shadowConstantBuffer)
        {
            return;
        }

        if (!material.GetVertexShader() || !material.GetPixelShader())
        {
            return;
        }

        Scene::Camera fallbackCamera;
        const Scene::Camera& camera = m_activeCamera ? *m_activeCamera : fallbackCamera;
        TransformBufferData transformData = {};
        MaterialBufferData materialData = {};
        LightBufferData lightData = {};
        ShadowBufferData shadowData = {};
        const float aspectRatio = m_height == 0 ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height);
        Scene::DirectionalLight fallbackLight;
        const Scene::DirectionalLight& light = m_directionalLight ? *m_directionalLight : fallbackLight;

        DirectX::XMStoreFloat4x4(&transformData.world, transform.GetWorldMatrix());
        DirectX::XMStoreFloat4x4(&transformData.view, camera.GetViewMatrix());
        DirectX::XMStoreFloat4x4(&transformData.projection, camera.GetProjectionMatrix(aspectRatio));
        materialData.baseColor = material.GetBaseColor();
        materialData.useBaseColorTexture = material.GetBaseColorTexture() ? 1 : 0;
        materialData.useRoughnessTexture = material.GetRoughnessTexture() ? 1 : 0;
        materialData.useMetallicTexture = material.GetMetallicTexture() ? 1 : 0;
        materialData.useNormalTexture = material.GetNormalTexture() ? 1 : 0;
        materialData.roughness = material.GetRoughness();
        materialData.metallic = material.GetMetallic();

        std::uint32_t lightCount = static_cast<std::uint32_t>(std::min<std::size_t>(m_lights.size(), MaxRenderLights));
        if (lightCount == 0)
        {
            RenderLight fallback;
            fallback.type = 0.0f;
            fallback.direction = light.direction;
            fallback.color = light.color;
            fallback.intensity = light.intensity;
            lightData.lights[0] = fallback;
            lightCount = 1;
            m_ambientColor = light.ambientColor;
        }
        else
        {
            for (std::uint32_t i = 0; i < lightCount; ++i)
            {
                lightData.lights[i] = m_lights[i];
            }
        }
        lightData.lightCount = lightCount;
        lightData.ambientColor = { m_ambientColor.x, m_ambientColor.y, m_ambientColor.z, 1.0f };
        if (m_shadowMap && m_shadowMap->IsReady())
        {
            shadowData.lightView = m_shadowMap->GetLightView();
            shadowData.lightProjection = m_shadowMap->GetLightProjection();
            shadowData.shadowParams.x = m_enableShadows ? 1.0f : 0.0f;
        }

        RHI::RHIContext& context = m_device->GetContext();
        context.UpdateConstantBuffer(m_transformConstantBuffer, &transformData, sizeof(transformData));
        context.UpdateConstantBuffer(m_materialConstantBuffer, &materialData, sizeof(materialData));
        context.UpdateConstantBuffer(m_lightConstantBuffer, &lightData, sizeof(lightData));
        context.UpdateConstantBuffer(m_shadowConstantBuffer, &shadowData, sizeof(shadowData));
        context.SetConstantBuffer(RHI::ShaderStage::Vertex, 0, m_transformConstantBuffer);
        context.SetConstantBuffer(RHI::ShaderStage::Vertex, 3, m_shadowConstantBuffer);
        context.SetConstantBuffer(RHI::ShaderStage::Pixel, 1, m_materialConstantBuffer);
        context.SetConstantBuffer(RHI::ShaderStage::Pixel, 2, m_lightConstantBuffer);
        context.SetConstantBuffer(RHI::ShaderStage::Pixel, 3, m_shadowConstantBuffer);
        context.SetTexture(RHI::ShaderStage::Pixel, 0, material.GetBaseColorTexture());
        if (m_enableShadows && m_shadowMap && m_shadowMap->IsReady())
        {
            context.SetTexture(RHI::ShaderStage::Pixel, 1, m_shadowMap->GetDepthTexture());
        }
        else
        {
            context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        }
        context.SetTexture(RHI::ShaderStage::Pixel, 2, material.GetRoughnessTexture());
        context.SetTexture(RHI::ShaderStage::Pixel, 3, material.GetMetallicTexture());
        context.SetTexture(RHI::ShaderStage::Pixel, 4, material.GetNormalTexture());
        context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
        context.SetVertexBuffer(mesh.GetVertexBuffer());
        context.SetIndexBuffer(mesh.GetIndexBuffer());
        context.SetVertexShader(material.GetVertexShader());
        context.SetPixelShader(material.GetPixelShader());
        context.DrawIndexed(mesh.GetIndexCount());
        ++m_drawCallCount;
        m_triangleCount += mesh.GetIndexCount() / 3;
    }

    void Renderer::SubmitMesh(const Mesh& mesh, const Material& material, const Math::Transform& transform)
    {
        m_renderQueue.Submit({ &mesh, &material, transform });
    }

    void Renderer::FlushRenderQueue()
    {
        std::vector<RenderItem> items = m_renderQueue.GetItems();
        std::stable_sort(items.begin(), items.end(), [](const RenderItem& a, const RenderItem& b)
        {
            if (a.material != b.material)
            {
                return a.material < b.material;
            }
            return a.mesh < b.mesh;
        });

        for (const RenderItem& item : items)
        {
            DrawMesh(*item.mesh, *item.material, item.transform);
        }
        m_renderQueue.Clear();
    }

    void Renderer::RenderShadowPass(const Scene::Scene& scene)
    {
        if (!m_enableShadows || !m_device || !m_shadowMap || !m_shadowMap->IsReady())
        {
            return;
        }

        if (m_directionalLight)
        {
            m_shadowMap->UpdateLightMatrices(m_directionalLight->direction);
        }

        RHI::RHIContext& context = m_device->GetContext();
        m_shadowMap->BeginShadowPass(context);
        Scene::Camera fallbackCamera;
        TransformBufferData transformData = {};
        DirectX::XMStoreFloat4x4(&transformData.view, DirectX::XMLoadFloat4x4(&m_shadowMap->GetLightView()));
        DirectX::XMStoreFloat4x4(&transformData.projection, DirectX::XMLoadFloat4x4(&m_shadowMap->GetLightProjection()));

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            const Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMesh())
            {
                continue;
            }

            const Mesh& mesh = *meshRenderer->GetMesh();
            DirectX::XMStoreFloat4x4(&transformData.world, object->GetTransform().GetWorldMatrix());
            context.UpdateConstantBuffer(m_transformConstantBuffer, &transformData, sizeof(transformData));
            context.SetConstantBuffer(RHI::ShaderStage::Vertex, 0, m_transformConstantBuffer);
            context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
            context.SetVertexBuffer(mesh.GetVertexBuffer());
            context.SetIndexBuffer(mesh.GetIndexBuffer());
            context.SetVertexShader(m_shadowMap->GetVertexShader());
            context.SetPixelShader(m_shadowMap->GetPixelShader());
            context.DrawIndexed(mesh.GetIndexCount());
        }
        m_shadowMap->EndShadowPass(context);

        context.SetRenderTarget(m_sceneColorTarget, m_sceneDepthTarget);
        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);
    }

    void Renderer::Render(const Scene::GameObject& object)
    {
        const Scene::MeshRendererComponent* meshRenderer = object.GetComponent<Scene::MeshRendererComponent>();
        if (!meshRenderer || !meshRenderer->GetMesh() || !meshRenderer->GetMaterial())
        {
            return;
        }

        SubmitMesh(*meshRenderer->GetMesh(), *meshRenderer->GetMaterial(), object.GetTransform());
    }

    void Renderer::RenderColliders(const Scene::Scene& scene, const Scene::GameObject* selectedObject, bool enabled)
    {
        if (!enabled || !m_debugRenderer || !m_device)
        {
            return;
        }

        Scene::Camera fallbackCamera;
        const Scene::Camera& camera = m_activeCamera ? *m_activeCamera : fallbackCamera;
        const float aspectRatio = m_height == 0 ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height);

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            const bool selected = selectedObject == object.get();
            const Math::Vector4 color = selected
                ? Math::Vector4{ 1.0f, 0.85f, 0.1f, 1.0f }
                : Math::Vector4{ 0.1f, 0.9f, 0.45f, 1.0f };

            for (const Physics::ColliderComponent* collider : object->GetComponents<Physics::ColliderComponent>())
            {
                if (!collider)
                {
                    continue;
                }

                if (collider->type == Physics::ColliderType::Sphere)
                {
                    m_debugRenderer->DrawSphere(collider->GetWorldCenter(), collider->GetWorldRadius(), color);
                }
                else
                {
                    m_debugRenderer->DrawBox(
                        collider->GetWorldCenter(),
                        collider->GetWorldSize(),
                        collider->GetWorldAxisX(),
                        collider->GetWorldAxisY(),
                        collider->GetWorldAxisZ(),
                        color);
                }
            }
        }

        m_debugRenderer->Flush(m_device->GetContext(), camera.GetViewMatrix(), camera.GetProjectionMatrix(aspectRatio));
    }

    void Renderer::RenderSelectionGizmo(const Scene::GameObject* selectedObject, bool enabled, GizmoVisualMode mode)
    {
        if (!enabled || !selectedObject || !m_debugRenderer || !m_device)
        {
            return;
        }

        Scene::Camera fallbackCamera;
        const Scene::Camera& camera = m_activeCamera ? *m_activeCamera : fallbackCamera;
        const float aspectRatio = m_height == 0 ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height);
        const Math::Transform& transform = selectedObject->GetTransform();
        const Math::Vector3 origin = transform.position;

        // Draw editor gizmos on top of scene geometry so nearby objects do not hide them.
        m_device->GetContext().ClearDepthStencil(1.0f, 0);

        constexpr float axisLength = 1.5f;
        constexpr float boxSize = 0.18f;
        constexpr float ringRadius = 1.15f;

        const Math::Vector3 axes[] =
        {
            RotateAxis(transform.rotation, { 1.0f, 0.0f, 0.0f }),
            RotateAxis(transform.rotation, { 0.0f, 1.0f, 0.0f }),
            RotateAxis(transform.rotation, { 0.0f, 0.0f, 1.0f })
        };
        const Math::Vector4 colors[] =
        {
            { 1.0f, 0.1f, 0.1f, 1.0f },
            { 0.1f, 1.0f, 0.1f, 1.0f },
            { 0.2f, 0.45f, 1.0f, 1.0f }
        };

        if (mode == GizmoVisualMode::Rotate)
        {
            m_debugRenderer->DrawCircle(origin, ringRadius, axes[1], axes[2], colors[0]);
            m_debugRenderer->DrawCircle(origin, ringRadius, axes[0], axes[2], colors[1]);
            m_debugRenderer->DrawCircle(origin, ringRadius, axes[0], axes[1], colors[2]);
        }
        else
        {
            for (int i = 0; i < 3; ++i)
            {
                const Math::Vector3 end = Add(origin, Scale(axes[i], axisLength));
                m_debugRenderer->DrawLine(origin, end, colors[i]);

                if (mode == GizmoVisualMode::Move)
                {
                    const Math::Vector3 arrowBase = Add(origin, Scale(axes[i], axisLength - 0.25f));
                    const Math::Vector3 sideA = Scale(axes[(i + 1) % 3], 0.12f);
                    const Math::Vector3 sideB = Scale(axes[(i + 2) % 3], 0.12f);
                    m_debugRenderer->DrawLine(end, Add(arrowBase, sideA), colors[i]);
                    m_debugRenderer->DrawLine(end, Add(arrowBase, Scale(sideA, -1.0f)), colors[i]);
                    m_debugRenderer->DrawLine(end, Add(arrowBase, sideB), colors[i]);
                    m_debugRenderer->DrawLine(end, Add(arrowBase, Scale(sideB, -1.0f)), colors[i]);
                }
                else if (mode == GizmoVisualMode::Scale)
                {
                    m_debugRenderer->DrawBox(end, { boxSize, boxSize, boxSize }, axes[0], axes[1], axes[2], colors[i]);
                }
            }
        }

        m_debugRenderer->DrawSphere(origin, 0.08f, { 1.0f, 0.9f, 0.1f, 1.0f });
        m_debugRenderer->Flush(m_device->GetContext(), camera.GetViewMatrix(), camera.GetProjectionMatrix(aspectRatio));
    }

    void Renderer::ApplyPostProcess()
    {
        if (!m_enablePostProcess || !m_device || !m_postProcessStack)
        {
            return;
        }

        RHI::RHIContext& context = m_device->GetContext();
        context.SetDefaultRenderTarget();
        context.ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f);
        context.ClearDepthStencil(1.0f, 0);
        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);
        m_postProcessStack->Apply(context, m_sceneColorTarget);
        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
    }

    void Renderer::SetPostProcessSettings(const PostProcessSettings& settings)
    {
        m_enablePostProcess = settings.enabled && settings.HasAnyEffectEnabled();
        if (m_postProcessStack)
        {
            m_postProcessStack->SetSettings(settings);
        }
    }

    PostProcessSettings Renderer::GetPostProcessSettings() const
    {
        if (m_postProcessStack)
        {
            return m_postProcessStack->GetSettings();
        }

        return {};
    }

    std::uint32_t Renderer::GetDrawCallCount() const
    {
        return m_drawCallCount;
    }

    std::uint32_t Renderer::GetTriangleCount() const
    {
        return m_triangleCount;
    }

    void* Renderer::GetNativeDeviceHandleForDebugUI() const
    {
        return m_device ? m_device->GetNativeHandleForDebugUI() : nullptr;
    }

    void* Renderer::GetNativeContextHandleForDebugUI() const
    {
        return m_device ? m_device->GetContext().GetNativeHandleForDebugUI() : nullptr;
    }

    bool Renderer::CreateRenderResources()
    {
        m_transformConstantBuffer = m_device->CreateConstantBuffer(sizeof(TransformBufferData));
        if (!m_transformConstantBuffer)
        {
            Core::Log::Error("Failed to create transform constant buffer.");
            return false;
        }

        m_materialConstantBuffer = m_device->CreateConstantBuffer(sizeof(MaterialBufferData));
        if (!m_materialConstantBuffer)
        {
            Core::Log::Error("Failed to create material constant buffer.");
            return false;
        }

        m_lightConstantBuffer = m_device->CreateConstantBuffer(sizeof(LightBufferData));
        if (!m_lightConstantBuffer)
        {
            Core::Log::Error("Failed to create light constant buffer.");
            return false;
        }

        m_shadowConstantBuffer = m_device->CreateConstantBuffer(sizeof(ShadowBufferData));
        if (!m_shadowConstantBuffer)
        {
            Core::Log::Error("Failed to create shadow constant buffer.");
            return false;
        }

        m_sceneColorTarget = m_device->CreateRenderTarget(m_width, m_height, RHI::TextureFormat::RGBA8);
        m_sceneDepthTarget = m_device->CreateDepthTarget(m_width, m_height, RHI::TextureFormat::D24S8, false);
        if (!m_sceneColorTarget || !m_sceneDepthTarget)
        {
            Core::Log::Error("Failed to create scene render targets.");
            return false;
        }

        std::shared_ptr<Material> defaultMaterial = CreateMaterial({ 1.0f, 1.0f, 1.0f, 1.0f });
        if (!defaultMaterial)
        {
            Core::Log::Error("Failed to create default material.");
            return false;
        }

        m_defaultMaterial = defaultMaterial;

        return true;
    }
}
