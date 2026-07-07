#include "ComponentRegistry.h"

#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Scene/LightComponent.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/PostProcessComponent.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace Engine::Scene
{
    namespace
    {
        std::unordered_map<std::string, ComponentRegistry::CreateFunc>& Registry()
        {
            static std::unordered_map<std::string, ComponentRegistry::CreateFunc> registry;
            return registry;
        }

        bool& DefaultsRegistered()
        {
            static bool registered = false;
            return registered;
        }

        void EnsureDefaultsRegistered()
        {
            if (!DefaultsRegistered())
            {
                ComponentRegistry::RegisterDefaultComponents();
            }
        }
    }

    void ComponentRegistry::Register(const std::string& typeName, CreateFunc createFunc)
    {
        if (typeName.empty() || !createFunc)
        {
            return;
        }

        Registry()[typeName] = std::move(createFunc);
    }

    std::shared_ptr<Component> ComponentRegistry::Create(const std::string& typeName)
    {
        EnsureDefaultsRegistered();
        const auto it = Registry().find(typeName);
        if (it == Registry().end())
        {
            return nullptr;
        }

        return it->second();
    }

    bool ComponentRegistry::IsRegistered(const std::string& typeName)
    {
        EnsureDefaultsRegistered();
        return Registry().find(typeName) != Registry().end();
    }

    std::vector<std::string> ComponentRegistry::GetRegisteredTypeNames()
    {
        EnsureDefaultsRegistered();
        std::vector<std::string> names;
        names.reserve(Registry().size());
        for (const auto& [name, createFunc] : Registry())
        {
            (void)createFunc;
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    std::string ComponentRegistry::GetTypeName(const Component& component)
    {
        return component.GetTypeName();
    }

    void ComponentRegistry::RegisterDefaultComponents()
    {
        if (DefaultsRegistered())
        {
            return;
        }

        DefaultsRegistered() = true;
        Register<MeshRendererComponent>(MeshRendererComponent::StaticTypeName);
        Register<LightComponent>(LightComponent::StaticTypeName);
        Register<Physics::ColliderComponent>(Physics::ColliderComponent::StaticTypeName);
        Register<PostProcessComponent>(PostProcessComponent::StaticTypeName);
        Register<PlayerStartComponent>(PlayerStartComponent::StaticTypeName);
    }
}
