// undo_redo_manager.cpp
#include "undo_redo_manager.h"
#include <godot_cpp/variant/utilities.hpp>
#include <godot_cpp/variant/utilities.hpp>

using namespace godot;

void UndoRedoManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_action", "name", "do_callable", "undo_callable"), &UndoRedoManager::add_action);
    ClassDB::bind_method(D_METHOD("undo"), &UndoRedoManager::undo);
    ClassDB::bind_method(D_METHOD("redo"), &UndoRedoManager::redo);
    ClassDB::bind_method(D_METHOD("clear"), &UndoRedoManager::clear);
    ClassDB::bind_method(D_METHOD("can_undo"), &UndoRedoManager::can_undo);
    ClassDB::bind_method(D_METHOD("can_redo"), &UndoRedoManager::can_redo);
    ClassDB::bind_method(D_METHOD("get_undo_name"), &UndoRedoManager::get_undo_name);
    ClassDB::bind_method(D_METHOD("get_redo_name"), &UndoRedoManager::get_redo_name);
    ClassDB::bind_method(D_METHOD("set_max_history", "max"), &UndoRedoManager::set_max_history);
    ClassDB::bind_method(D_METHOD("get_max_history"), &UndoRedoManager::get_max_history);
    ClassDB::add_property("UndoRedoManager", PropertyInfo(Variant::INT, "max_history"), "set_max_history", "get_max_history");

    ADD_SIGNAL(MethodInfo("history_changed"));
}

UndoRedoManager::UndoRedoManager() {}

void UndoRedoManager::add_action(const String &p_name, const Callable &p_do, const Callable &p_undo) {
    while (current_index < (int)history.size() - 1) {
        history.remove_at(history.size() - 1);
    }
    if ((int)history.size() >= max_history) {
        history.remove_at(0);
        current_index--;
    }

    Action action;
    action.name = p_name;
    action.do_callable = p_do;
    action.undo_callable = p_undo;
    history.push_back(action);
    current_index++;

    Variant result;
    GDExtensionCallError err;
    action.do_callable.callp(nullptr, 0, result, err);
    if (err.error != GDEXTENSION_CALL_OK) {
        UtilityFunctions::push_error("[UndoRedoManager] Do failed: ", p_name);
    }
    emit_signal("history_changed");
}

void UndoRedoManager::undo() {
    if (!can_undo()) return;
    Action &action = history.write[current_index];
    Variant result;
    GDExtensionCallError err;
    action.undo_callable.callp(nullptr, 0, result, err);
    if (err.error != GDEXTENSION_CALL_OK) {
        UtilityFunctions::push_error("[UndoRedoManager] Undo failed: ", action.name);
    }
    current_index--;
    emit_signal("history_changed");
}

void UndoRedoManager::redo() {
    if (!can_redo()) return;
    current_index++;
    Action &action = history.write[current_index];
    Variant result;
    GDExtensionCallError err;
    action.do_callable.callp(nullptr, 0, result, err);
    if (err.error != GDEXTENSION_CALL_OK) {
        UtilityFunctions::push_error("[UndoRedoManager] Redo failed: ", action.name);
    }
    emit_signal("history_changed");
}

void UndoRedoManager::clear() {
    history.clear();
    current_index = -1;
    emit_signal("history_changed");
}

bool UndoRedoManager::can_undo() const { return current_index >= 0; }
bool UndoRedoManager::can_redo() const { return current_index < (int)history.size() - 1; }

String UndoRedoManager::get_undo_name() const {
    if (!can_undo()) return "";
    return history[current_index].name;
}
String UndoRedoManager::get_redo_name() const {
    if (!can_redo()) return "";
    return history[current_index + 1].name;
}

void UndoRedoManager::set_max_history(int p_max) { max_history = Math::max(1, p_max); }
int UndoRedoManager::get_max_history() const { return max_history; }
