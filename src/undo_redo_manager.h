// undo_redo_manager.h
#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class UndoRedoManager : public RefCounted {
    GDCLASS(UndoRedoManager, RefCounted)

private:
    struct Action {
        String name;
        Callable do_callable;
        Callable undo_callable;
    };

    Vector<Action> history;
    int current_index = -1;
    int max_history = 50;

protected:
    static void _bind_methods();

public:
    void add_action(const String &p_name, const Callable &p_do, const Callable &p_undo);
    void undo();
    void redo();
    void clear();
    bool can_undo() const;
    bool can_redo() const;
    String get_undo_name() const;
    String get_redo_name() const;
    void set_max_history(int p_max);
    int get_max_history() const;

    UndoRedoManager();
};

}

#endif
