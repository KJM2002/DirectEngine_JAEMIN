#pragma once

#include "Component.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Engine::Scene
{
    class ComponentRegistry
    {
    public:
        using CreateFunc = std::function<std::shared_ptr<Component>()>;

        template <typename T>
        static void Register(const std::string& typeName)
        {
            Register(typeName, []()
            {
                return std::make_shared<T>();
            });
        }

        static void Register(const std::string& typeName, CreateFunc createFunc);
        static std::shared_ptr<Component> Create(const std::string& typeName);
        static bool IsRegistered(const std::string& typeName);
        static std::vector<std::string> GetRegisteredTypeNames();
        static std::string GetTypeName(const Component& component);
        static void RegisterDefaultComponents();
    };
}
