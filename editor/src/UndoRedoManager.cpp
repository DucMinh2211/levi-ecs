#include "UndoRedoManager.h"

namespace Levi {

    LambdaCommand::LambdaCommand(std::string name, std::function<void()> undo, std::function<void()> redo)
        : name_(std::move(name)), undo_(std::move(undo)), redo_(std::move(redo)) {}

    void LambdaCommand::undo() {
        if (undo_) undo_();
    }

    void LambdaCommand::redo() {
        if (redo_) redo_();
    }

    void UndoRedoManager::execute(std::unique_ptr<EditorCommand> command) {
        if (!command) return;
        command->redo();
        record(std::move(command));
    }

    void UndoRedoManager::record(std::unique_ptr<EditorCommand> command) {
        if (!command) return;
        redoStack_.clear();
        pushUndo(std::move(command));
    }

    bool UndoRedoManager::undo() {
        if (undoStack_.empty()) return false;
        auto command = std::move(undoStack_.back());
        undoStack_.pop_back();
        command->undo();
        redoStack_.push_back(std::move(command));
        return true;
    }

    bool UndoRedoManager::redo() {
        if (redoStack_.empty()) return false;
        auto command = std::move(redoStack_.back());
        redoStack_.pop_back();
        command->redo();
        pushUndo(std::move(command));
        return true;
    }

    void UndoRedoManager::clear() {
        undoStack_.clear();
        redoStack_.clear();
    }

    const char* UndoRedoManager::undoName() const {
        return canUndo() ? undoStack_.back()->name().c_str() : nullptr;
    }

    const char* UndoRedoManager::redoName() const {
        return canRedo() ? redoStack_.back()->name().c_str() : nullptr;
    }

    void UndoRedoManager::pushUndo(std::unique_ptr<EditorCommand> command) {
        if (maxHistory_ == 0) return;
        if (undoStack_.size() == maxHistory_) {
            undoStack_.erase(undoStack_.begin());
        }
        undoStack_.push_back(std::move(command));
    }

} // namespace Levi
