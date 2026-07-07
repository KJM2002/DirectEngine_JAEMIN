#include "PropertySystem.h"

#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Editor/Property/PropertyChangeCommand.h"
#include "Engine/Graphics/Renderer/Material.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/Scene.h"

#include <cmath>
#include <memory>

namespace Engine::Editor
{
    namespace
    {
        bool NearlyEqual(float a, float b, float epsilon)
        {
            return std::abs(a - b) <= epsilon;
        }

        bool NearlyEqual(const Math::Vector3& a, const Math::Vector3& b, float epsilon)
        {
            return NearlyEqual(a.x, b.x, epsilon)
                && NearlyEqual(a.y, b.y, epsilon)
                && NearlyEqual(a.z, b.z, epsilon);
        }

        bool NearlyEqual(const Math::Vector4& a, const Math::Vector4& b, float epsilon)
        {
            return NearlyEqual(a.x, b.x, epsilon)
                && NearlyEqual(a.y, b.y, epsilon)
                && NearlyEqual(a.z, b.z, epsilon)
                && NearlyEqual(a.w, b.w, epsilon);
        }

        Scene::MeshRendererComponent* FindMeshRenderer(Scene::Scene& scene, Scene::ObjectID objectId, Scene::ComponentID componentId)
        {
            if (Scene::Component* component = scene.FindComponentByID(componentId))
            {
                return dynamic_cast<Scene::MeshRendererComponent*>(component);
            }

            Scene::GameObject* object = scene.FindGameObjectByID(objectId);
            return object ? object->GetComponent<Scene::MeshRendererComponent>() : nullptr;
        }

        const Scene::MeshRendererComponent* FindMeshRenderer(const Scene::Scene& scene, Scene::ObjectID objectId, Scene::ComponentID componentId)
        {
            return FindMeshRenderer(const_cast<Scene::Scene&>(scene), objectId, componentId);
        }
    }

    bool PropertySystem::SetProperty(CommandManager& commandManager,
        Scene::Scene& scene,
        Scene::ObjectID objectId,
        Scene::ComponentID componentId,
        const std::string& propertyPath,
        const PropertyValue& newValue)
    {
        PropertyValue oldValue;
        if (!GetProperty(scene, objectId, componentId, propertyPath, oldValue))
        {
            return false;
        }

        return SetProperty(commandManager, scene, objectId, componentId, propertyPath, oldValue, newValue);
    }

    bool PropertySystem::SetProperty(CommandManager& commandManager,
        Scene::Scene& scene,
        Scene::ObjectID objectId,
        Scene::ComponentID componentId,
        const std::string& propertyPath,
        const PropertyValue& oldValue,
        const PropertyValue& newValue)
    {
        if (AreEqual(oldValue, newValue))
        {
            return false;
        }

        commandManager.ExecuteCommand(std::make_unique<PropertyChangeCommand>(scene, objectId, componentId, propertyPath, oldValue, newValue));
        return true;
    }

    bool PropertySystem::GetProperty(const Scene::Scene& scene,
        Scene::ObjectID objectId,
        Scene::ComponentID componentId,
        const std::string& propertyPath,
        PropertyValue& outValue)
    {
        const Scene::GameObject* object = scene.FindGameObjectByID(objectId);
        if (!object)
        {
            return false;
        }

        if (propertyPath == "Transform.Position")
        {
            outValue = object->GetTransform().position;
            return true;
        }
        if (propertyPath == "Transform.Rotation")
        {
            outValue = object->GetTransform().rotation;
            return true;
        }
        if (propertyPath == "Transform.Scale")
        {
            outValue = object->GetTransform().scale;
            return true;
        }

        const Scene::MeshRendererComponent* meshRenderer = FindMeshRenderer(scene, objectId, componentId);
        const Renderer::Material* material = meshRenderer && meshRenderer->GetMaterial()
            ? meshRenderer->GetMaterial().get()
            : nullptr;
        if (!material)
        {
            return false;
        }

        if (propertyPath == "MeshRenderer.Material.BaseColor")
        {
            outValue = material->GetBaseColor();
            return true;
        }
        if (propertyPath == "MeshRenderer.Material.Roughness")
        {
            outValue = material->GetRoughness();
            return true;
        }
        if (propertyPath == "MeshRenderer.Material.Metallic")
        {
            outValue = material->GetMetallic();
            return true;
        }

        return false;
    }

    bool PropertySystem::ApplyProperty(Scene::Scene& scene,
        Scene::ObjectID objectId,
        Scene::ComponentID componentId,
        const std::string& propertyPath,
        const PropertyValue& value)
    {
        Scene::GameObject* object = scene.FindGameObjectByID(objectId);
        if (!object)
        {
            return false;
        }

        if (propertyPath == "Transform.Position")
        {
            if (const Math::Vector3* position = std::get_if<Math::Vector3>(&value))
            {
                object->GetTransform().position = *position;
                return true;
            }
            return false;
        }
        if (propertyPath == "Transform.Rotation")
        {
            if (const Math::Vector3* rotation = std::get_if<Math::Vector3>(&value))
            {
                object->GetTransform().rotation = *rotation;
                return true;
            }
            return false;
        }
        if (propertyPath == "Transform.Scale")
        {
            if (const Math::Vector3* scale = std::get_if<Math::Vector3>(&value))
            {
                object->GetTransform().scale = *scale;
                return true;
            }
            return false;
        }

        Scene::MeshRendererComponent* meshRenderer = FindMeshRenderer(scene, objectId, componentId);
        Renderer::Material* material = meshRenderer && meshRenderer->GetMaterial()
            ? meshRenderer->GetMaterial().get()
            : nullptr;
        if (!material)
        {
            return false;
        }

        if (propertyPath == "MeshRenderer.Material.BaseColor")
        {
            if (const Math::Vector4* color = std::get_if<Math::Vector4>(&value))
            {
                material->SetBaseColor(*color);
                return true;
            }
            return false;
        }
        if (propertyPath == "MeshRenderer.Material.Roughness")
        {
            if (const float* roughness = std::get_if<float>(&value))
            {
                material->SetRoughness(*roughness);
                return true;
            }
            return false;
        }
        if (propertyPath == "MeshRenderer.Material.Metallic")
        {
            if (const float* metallic = std::get_if<float>(&value))
            {
                material->SetMetallic(*metallic);
                return true;
            }
            return false;
        }

        return false;
    }

    bool PropertySystem::AreEqual(const PropertyValue& a, const PropertyValue& b, float epsilon)
    {
        if (a.index() != b.index())
        {
            return false;
        }

        if (const float* av = std::get_if<float>(&a))
        {
            return NearlyEqual(*av, std::get<float>(b), epsilon);
        }
        if (const bool* av = std::get_if<bool>(&a))
        {
            return *av == std::get<bool>(b);
        }
        if (const int* av = std::get_if<int>(&a))
        {
            return *av == std::get<int>(b);
        }
        if (const Math::Vector3* av = std::get_if<Math::Vector3>(&a))
        {
            return NearlyEqual(*av, std::get<Math::Vector3>(b), epsilon);
        }
        if (const Math::Vector4* av = std::get_if<Math::Vector4>(&a))
        {
            return NearlyEqual(*av, std::get<Math::Vector4>(b), epsilon);
        }
        if (const std::string* av = std::get_if<std::string>(&a))
        {
            return *av == std::get<std::string>(b);
        }

        return false;
    }
}
