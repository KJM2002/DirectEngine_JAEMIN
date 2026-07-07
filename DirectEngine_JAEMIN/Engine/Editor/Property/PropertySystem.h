#pragma once

#include "Engine/Editor/Property/PropertyValue.h"
#include "Engine/Scene/SceneIDs.h"

#include <string>

namespace Engine::Scene
{
    class Scene;
}

namespace Engine::Editor
{
    class CommandManager;

    class PropertySystem
    {
    public:
        static bool SetProperty(CommandManager& commandManager,
            Scene::Scene& scene,
            Scene::ObjectID objectId,
            Scene::ComponentID componentId,
            const std::string& propertyPath,
            const PropertyValue& newValue);

        static bool SetProperty(CommandManager& commandManager,
            Scene::Scene& scene,
            Scene::ObjectID objectId,
            Scene::ComponentID componentId,
            const std::string& propertyPath,
            const PropertyValue& oldValue,
            const PropertyValue& newValue);

        static bool GetProperty(const Scene::Scene& scene,
            Scene::ObjectID objectId,
            Scene::ComponentID componentId,
            const std::string& propertyPath,
            PropertyValue& outValue);

        static bool ApplyProperty(Scene::Scene& scene,
            Scene::ObjectID objectId,
            Scene::ComponentID componentId,
            const std::string& propertyPath,
            const PropertyValue& value);

        static bool AreEqual(const PropertyValue& a, const PropertyValue& b, float epsilon = 0.0001f);
    };
}
