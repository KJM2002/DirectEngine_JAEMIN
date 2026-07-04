#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace Engine::Scene
{
    class Scene;
}

namespace Engine::Editor
{
    class ICommand;

    class CommandManager
    {
    public:
        explicit CommandManager(Scene::Scene& scene);

        void ExecuteCommand(std::unique_ptr<ICommand> command);
        void Undo();
        void Redo();
        void Clear();

        bool CanUndo() const;
        bool CanRedo() const;
        const char* GetUndoName() const;
        const char* GetRedoName() const;
        std::uint64_t GetCurrentRevision() const;
        std::uint64_t GetSavedRevision() const;
        void MarkSaved();
        bool IsDirtySinceSave() const;

    private:
        Scene::Scene& m_scene;
        std::vector<std::unique_ptr<ICommand>> m_undoStack;
        std::vector<std::unique_ptr<ICommand>> m_redoStack;
        std::uint64_t m_currentRevision = 0;
        std::uint64_t m_savedRevision = 0;
    };
}
