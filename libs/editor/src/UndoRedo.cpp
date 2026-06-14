#include "Hockey/Editor/UndoRedo.hpp"

#include <utility>

namespace Hockey {

void UndoRedoStack::Execute(std::unique_ptr<EditorCommand> command, EditorContext& context) {
    if (!command) {
        return;
    }
    command->Execute(context);
    m_RedoStack.clear();
    m_UndoStack.push_back(std::move(command));
    while (m_UndoStack.size() > m_Capacity && !m_UndoStack.empty()) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}

void UndoRedoStack::Undo(EditorContext& context) {
    if (m_UndoStack.empty()) {
        return;
    }
    std::unique_ptr<EditorCommand> command = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();
    command->Undo(context);
    m_RedoStack.push_back(std::move(command));
}

void UndoRedoStack::Redo(EditorContext& context) {
    if (m_RedoStack.empty()) {
        return;
    }
    std::unique_ptr<EditorCommand> command = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();
    command->Execute(context);
    m_UndoStack.push_back(std::move(command));
}

bool UndoRedoStack::CanUndo() const {
    return !m_UndoStack.empty();
}

bool UndoRedoStack::CanRedo() const {
    return !m_RedoStack.empty();
}

std::string UndoRedoStack::UndoName() const {
    return m_UndoStack.empty() ? std::string() : m_UndoStack.back()->Name();
}

std::string UndoRedoStack::RedoName() const {
    return m_RedoStack.empty() ? std::string() : m_RedoStack.back()->Name();
}

void UndoRedoStack::Clear() {
    m_UndoStack.clear();
    m_RedoStack.clear();
}

std::size_t UndoRedoStack::UndoCount() const {
    return m_UndoStack.size();
}

std::size_t UndoRedoStack::RedoCount() const {
    return m_RedoStack.size();
}

void UndoRedoStack::SetCapacity(std::size_t capacity) {
    m_Capacity = capacity == 0 ? 1 : capacity;
    while (m_UndoStack.size() > m_Capacity && !m_UndoStack.empty()) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}

} // namespace Hockey
