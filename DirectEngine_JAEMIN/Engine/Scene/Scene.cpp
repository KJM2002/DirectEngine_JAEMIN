#include "Scene.h"

#include "Engine/Graphics/Renderer/Renderer.h"
#include "Camera.h"
#include "Light.h"
#include "LightComponent.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <utility>

namespace Engine::Scene
{
    GameObject& Scene::CreateGameObject(const std::string& name)
    {
        auto object = std::make_unique<GameObject>(name);
        GameObject& result = *object;
        m_gameObjects.push_back(std::move(object));
        return result;
    }

    GameObject& Scene::CreateGameObject(const std::string& name, ObjectID id)
    {
        GameObject& object = CreateGameObject(name);
        object.SetID(id);
        return object;
    }

    bool Scene::DestroyGameObject(GameObject* object)
    {
        auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(), [object](const std::unique_ptr<GameObject>& candidate)
        {
            return candidate.get() == object;
        });

        if (it == m_gameObjects.end())
        {
            return false;
        }

        m_gameObjects.erase(it);
        return true;
    }

    GameObject* Scene::FindGameObjectByID(ObjectID id) const
    {
        if (id == InvalidObjectID)
        {
            return nullptr;
        }

        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            if (object && object->GetID() == id)
            {
                return object.get();
            }
        }

        return nullptr;
    }

    Component* Scene::FindComponentByID(ComponentID id) const
    {
        if (id == InvalidComponentID)
        {
            return nullptr;
        }

        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            if (!object)
            {
                continue;
            }

            if (Component* component = object->GetComponentByID(id))
            {
                return component;
            }
        }

        return nullptr;
    }

    void Scene::Update(float deltaTime)
    {
        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            object->Update(deltaTime);
        }
    }

    void Scene::Render(Renderer::Renderer& renderer)
    {
        renderer.SetActiveCamera(m_activeCamera.get());
        renderer.SetDirectionalLight(m_directionalLight.get());

        std::vector<Renderer::RenderLight> lights;
        Math::Vector3 ambientColor = m_directionalLight ? m_directionalLight->ambientColor : Math::Vector3{ 0.18f, 0.20f, 0.24f };

        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            const LightComponent* lightComponent = object->GetComponent<LightComponent>();
            if (!lightComponent)
            {
                continue;
            }

            Renderer::RenderLight light;
            light.position = object->GetTransform().position;
            light.range = lightComponent->range;
            light.direction = lightComponent->direction;
            light.type = static_cast<float>(lightComponent->type);
            light.color = lightComponent->color;
            light.intensity = lightComponent->intensity;
            const float innerRadians = DirectX::XMConvertToRadians(lightComponent->innerConeAngle);
            const float outerRadians = DirectX::XMConvertToRadians(lightComponent->outerConeAngle);
            light.spotAngles = { std::cos(innerRadians), std::cos(outerRadians), 0.0f };
            lights.push_back(light);

            if (lightComponent->type == LightType::Directional)
            {
                ambientColor = lightComponent->ambientColor;
            }
        }

        renderer.SetLights(lights, ambientColor);

        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            renderer.Render(*object);
        }
        renderer.FlushRenderQueue();
    }

    void Scene::Clear()
    {
        m_gameObjects.clear();
        m_outlinerFolders.clear();
        m_activeCamera.reset();
        m_directionalLight.reset();
        m_filePath.clear();
        m_dirty = false;
    }

    void Scene::SetActiveCamera(std::shared_ptr<Camera> camera)
    {
        m_activeCamera = std::move(camera);
    }

    const std::shared_ptr<Camera>& Scene::GetActiveCamera() const
    {
        return m_activeCamera;
    }

    void Scene::SetDirectionalLight(std::shared_ptr<DirectionalLight> light)
    {
        m_directionalLight = std::move(light);
    }

    const std::shared_ptr<DirectionalLight>& Scene::GetDirectionalLight() const
    {
        return m_directionalLight;
    }

    void Scene::EnsureDefaultCameraAndLight()
    {
        if (!m_activeCamera)
        {
            m_activeCamera = std::make_shared<Camera>();
        }

        if (!m_directionalLight)
        {
            m_directionalLight = std::make_shared<DirectionalLight>();
        }
    }

    const std::vector<std::unique_ptr<GameObject>>& Scene::GetGameObjects() const
    {
        return m_gameObjects;
    }

    const std::vector<std::string>& Scene::GetOutlinerFolders() const
    {
        return m_outlinerFolders;
    }

    bool Scene::HasOutlinerFolder(const std::string& folder) const
    {
        return std::find(m_outlinerFolders.begin(), m_outlinerFolders.end(), folder) != m_outlinerFolders.end();
    }

    void Scene::AddOutlinerFolder(std::string folder)
    {
        if (folder.empty() || HasOutlinerFolder(folder))
        {
            return;
        }

        m_outlinerFolders.push_back(std::move(folder));
    }

    void Scene::RenameOutlinerFolder(const std::string& oldFolder, std::string newFolder)
    {
        if (oldFolder.empty() || newFolder.empty() || oldFolder == newFolder || HasOutlinerFolder(newFolder))
        {
            return;
        }

        auto it = std::find(m_outlinerFolders.begin(), m_outlinerFolders.end(), oldFolder);
        if (it == m_outlinerFolders.end())
        {
            return;
        }

        *it = newFolder;
        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            if (object->GetOutlinerFolder() == oldFolder)
            {
                object->SetOutlinerFolder(newFolder);
            }
        }
    }

    void Scene::RemoveOutlinerFolder(const std::string& folder)
    {
        m_outlinerFolders.erase(std::remove(m_outlinerFolders.begin(), m_outlinerFolders.end(), folder), m_outlinerFolders.end());
        for (const std::unique_ptr<GameObject>& object : m_gameObjects)
        {
            if (object->GetOutlinerFolder() == folder)
            {
                object->SetOutlinerFolder({});
            }
        }
    }

    bool Scene::IsDirty() const
    {
        return m_dirty;
    }

    void Scene::MarkDirty()
    {
        m_dirty = true;
    }

    void Scene::ClearDirty()
    {
        m_dirty = false;
    }

    const std::filesystem::path& Scene::GetFilePath() const
    {
        return m_filePath;
    }

    void Scene::SetFilePath(const std::filesystem::path& path)
    {
        m_filePath = path.lexically_normal();
    }

    std::string Scene::GetDisplayName() const
    {
        if (m_filePath.empty())
        {
            return "Untitled.scene";
        }
        return m_filePath.filename().string();
    }

    std::string Scene::GetDirtyDisplayName() const
    {
        return IsDirty() ? "*" + GetDisplayName() : GetDisplayName();
    }
}
