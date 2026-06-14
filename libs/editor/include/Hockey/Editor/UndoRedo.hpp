#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Hockey {

class EditorContext;

// Base class for every reversible editor operation. A command captures the data
// it needs to (re)apply and to undo a single change. Commands run against the
// shared EditorContext (active scene + selection) so they can repair selection
// and mark the scene dirty as part of execution.
class EditorCommand {
public:
    virtual ~EditorCommand() = default;

    virtual void Execute(EditorContext& context) = 0;
    virtual void Undo(EditorContext& context) = 0;
    virtual std::string Name() const = 0;
};

// LIFO undo/redo history. Executing a fresh command clears the redo branch.
// A capacity bound keeps long editing sessions from growing without limit; the
// oldest entries are dropped first.
class UndoRedoStack {
public:
    // Runs command.Execute then records it for undo. Clears the redo stack.
    void Execute(std::unique_ptr<EditorCommand> command, EditorContext& context);

    void Undo(EditorContext& context);
    void Redo(EditorContext& context);

    bool CanUndo() const;
    bool CanRedo() const;

    // Name of the command that Undo()/Redo() would act on, or "" when empty.
    std::string UndoName() const;
    std::string RedoName() const;

    void Clear();

    std::size_t UndoCount() const;
    std::size_t RedoCount() const;

    std::size_t Capacity() const {
        return m_Capacity;
    }
    void SetCapacity(std::size_t capacity);

private:
    std::vector<std::unique_ptr<EditorCommand>> m_UndoStack;
    std::vector<std::unique_ptr<EditorCommand>> m_RedoStack;
    std::size_t m_Capacity = 256;
};

} // namespace Hockey
