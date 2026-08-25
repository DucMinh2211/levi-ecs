#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Levi {

    class EditorCommand {
    public:
        virtual ~EditorCommand() = default;
        virtual void undo() = 0;
        virtual void redo() = 0;
        virtual const std::string& name() const = 0;
    };

    class LambdaCommand final : public EditorCommand {
    public:
        LambdaCommand(std::string name, std::function<void()> undo, std::function<void()> redo);

        void undo() override;
        void redo() override;
        const std::string& name() const override { return name_; }

    private:
        std::string name_;
        std::function<void()> undo_;
        std::function<void()> redo_;
    };

    class UndoRedoManager {
    public:
        explicit UndoRedoManager(std::size_t maxHistory = 256) : maxHistory_(maxHistory) {}

        // Executes a new command and adds it to history.
        void execute(std::unique_ptr<EditorCommand> command);

        // Adds a command for a change already applied by an interactive widget.
        void record(std::unique_ptr<EditorCommand> command);

        bool undo();
        bool redo();
        void clear();

        bool canUndo() const { return !undoStack_.empty(); }
        bool canRedo() const { return !redoStack_.empty(); }
        const char* undoName() const;
        const char* redoName() const;

    private:
        void pushUndo(std::unique_ptr<EditorCommand> command);

        std::size_t maxHistory_;
        std::vector<std::unique_ptr<EditorCommand>> undoStack_;
        std::vector<std::unique_ptr<EditorCommand>> redoStack_;
    };

} // namespace Levi
