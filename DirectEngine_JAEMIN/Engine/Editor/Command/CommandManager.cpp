#include "CommandManager.h"

#include "ICommand.h"
#include "Engine/Scene/Scene.h"

namespace Engine::Editor
{
    CommandManager::CommandManager(Scene::Scene& scene)
        : m_scene(scene)
    {
    }

    void CommandManager::ExecuteCommand(std::unique_ptr<ICommand> command)
    {
        if (!command)
        {
            return;
        }

        command->Execute();
        m_undoStack.push_back(std::move(command));
        m_redoStack.clear();
        ++m_currentRevision;
        IsDirtySinceSave() ? m_scene.MarkDirty() : m_scene.ClearDirty();
    }

    void CommandManager::Undo()
    {
        if (m_undoStack.empty())
        {
            return;
        }

        std::unique_ptr<ICommand> command = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        command->Undo();
        m_redoStack.push_back(std::move(command));
        if (m_currentRevision > 0)
        {
            --m_currentRevision;
        }
        IsDirtySinceSave() ? m_scene.MarkDirty() : m_scene.ClearDirty();
    }

    void CommandManager::Redo()
    {
        if (m_redoStack.empty())
        {
            return;
        }

        std::unique_ptr<ICommand> command = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        command->Execute();
        m_undoStack.push_back(std::move(command));
        ++m_currentRevision;
        IsDirtySinceSave() ? m_scene.MarkDirty() : m_scene.ClearDirty();
    }

    void CommandManager::Clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
        m_currentRevision = 0;
        m_savedRevision = 0;
    }

    bool CommandManager::CanUndo() const
    {
        return !m_undoStack.empty();
    }

    bool CommandManager::CanRedo() const
    {
        return !m_redoStack.empty();
    }

    const char* CommandManager::GetUndoName() const
    {
        return CanUndo() ? m_undoStack.back()->GetName() : "";
    }

    const char* CommandManager::GetRedoName() const
    {
        return CanRedo() ? m_redoStack.back()->GetName() : "";
    }

    std::uint64_t CommandManager::GetCurrentRevision() const
    {
        return m_currentRevision;
    }

    std::uint64_t CommandManager::GetSavedRevision() const
    {
        return m_savedRevision;
    }

    void CommandManager::MarkSaved()
    {
        m_savedRevision = m_currentRevision;
        m_scene.ClearDirty();
    }

    bool CommandManager::IsDirtySinceSave() const
    {
        return m_currentRevision != m_savedRevision;
    }
}
