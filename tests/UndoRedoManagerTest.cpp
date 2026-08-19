#include "UndoRedoManager.h"

#include <memory>

int main() {
    Levi::UndoRedoManager history(2);
    int value = 0;

    auto setValue = [&history, &value](int next) {
        const int previous = value;
        history.execute(std::make_unique<Levi::LambdaCommand>(
            "Set Value",
            [&value, previous]() { value = previous; },
            [&value, next]() { value = next; }));
    };

    setValue(1);
    setValue(2);
    if (value != 2 || !history.undo() || value != 1) return 1;
    if (!history.redo() || value != 2) return 2;
    if (!history.undo()) return 3;

    setValue(3);
    if (history.canRedo() || value != 3) return 4;

    setValue(4); // The history limit drops the oldest command.
    if (!history.undo() || value != 3) return 5;
    if (!history.undo() || value != 1) return 6;
    if (history.undo()) return 7;

    history.clear();
    if (history.canUndo() || history.canRedo()) return 8;
    return 0;
}
