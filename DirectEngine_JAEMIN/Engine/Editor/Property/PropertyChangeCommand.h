#pragma once

#include "Engine/Editor/Command/ICommand.h"
#include "Engine/Editor/Property/PropertyValue.h"
#include "Engine/Scene/SceneIDs.h"

#include <string>

namespace Engine::Scene
{
    class Scene;
}

namespace Engine::Editor
{
    class PropertyChangeCommand : public ICommand
    {
    public:
        PropertyChangeCommand(Scene::Scene& scene,
            Scene::ObjectID objectId,
            Scene::ComponentID componentId,
            std::string propertyPath,
            PropertyValue oldValue,
            PropertyValue newValue);

        void Execute() override;
        void Undo() override;
        const char* GetName() const override;

    private:
        Scene::Scene& m_scene;
        Scene::ObjectID m_objectId = Scene::InvalidObjectID;
        Scene::ComponentID m_componentId = Scene::InvalidComponentID;
        std::string m_propertyPath;
        PropertyValue m_oldValue;
        PropertyValue m_newValue;
    };
}
