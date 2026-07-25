#include "transition_handle_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/global_constants.hpp>

using namespace godot;

void TransitionHandleNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_transition_duration", "duration"), &TransitionHandleNode::set_transition_duration);
    ClassDB::bind_method(D_METHOD("get_transition_duration"), &TransitionHandleNode::get_transition_duration);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "transition_duration"), "set_transition_duration", "get_transition_duration");

    ClassDB::bind_method(D_METHOD("set_max_duration", "max_duration"), &TransitionHandleNode::set_max_duration);
    ClassDB::bind_method(D_METHOD("get_max_duration"), &TransitionHandleNode::get_max_duration);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "max_duration"), "set_max_duration", "get_max_duration");

    ClassDB::bind_method(D_METHOD("set_handle_color", "color"), &TransitionHandleNode::set_handle_color);
    ClassDB::bind_method(D_METHOD("get_handle_color"), &TransitionHandleNode::get_handle_color);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::COLOR, "handle_color"), "set_handle_color", "get_handle_color");

    ADD_SIGNAL(MethodInfo("duration_changed", PropertyInfo(Variant::FLOAT, "new_duration")));
    ADD_SIGNAL(MethodInfo("drag_started"));
    ADD_SIGNAL(MethodInfo("drag_ended"));
}

TransitionHandleNode::TransitionHandleNode() {
    dragging = false;
    drag_start_x = 0.0f;
    transition_duration = 0.0f;
    max_duration = 5.0f; 
    handle_color = Color(0.0, 1.0, 0.0, 0.8); // Green tint matching previous style

    set_mouse_filter(MOUSE_FILTER_PASS);
    set_default_cursor_shape(CURSOR_HSIZE);
    set_custom_minimum_size(Vector2(15, 0)); // Minimum width for a grabbable area
}

TransitionHandleNode::~TransitionHandleNode() {}

void TransitionHandleNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        
        // Draw the main handle body
        draw_rect(Rect2(0, 0, size.x, size.y), handle_color, true);
        
        // Draw a white outline for visibility against dark timelines
        draw_rect(Rect2(0, 0, size.x, size.y), Color(1.0, 1.0, 1.0, 1.0), false, 1.0);
        
        // Draw two little grip lines in the center
        float center_x = size.x / 2.0f;
        draw_line(Vector2(center_x - 2, size.y * 0.3f), Vector2(center_x - 2, size.y * 0.7f), Color(1.0, 1.0, 1.0, 0.5), 1.0);
        draw_line(Vector2(center_x + 2, size.y * 0.3f), Vector2(center_x + 2, size.y * 0.7f), Color(1.0, 1.0, 1.0, 0.5), 1.0);
    }
}

void TransitionHandleNode::_gui_input(const Ref<InputEvent> &p_event) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (mb->is_pressed()) {
            dragging = true;
            drag_start_x = get_global_mouse_position().x;
            emit_signal("drag_started");
            accept_event();
        } else if (dragging) {
            dragging = false;
            emit_signal("drag_ended");
            accept_event();
        }
    }

    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && dragging) {
        float current_x = get_global_mouse_position().x;
        float delta_x = current_x - drag_start_x;
        drag_start_x = current_x; 
        
        // Timeline scale multiplier: pixels per second. 
        // Note: You can link this to your TimelineZoomController later.
        float pixels_per_second = 100.0f; 
        
        float delta_time = delta_x / pixels_per_second;
        float new_duration = CLAMP(transition_duration + delta_time, 0.0f, max_duration);
        
        if (new_duration != transition_duration) {
            set_transition_duration(new_duration);
            emit_signal("duration_changed", transition_duration);
        }
        accept_event();
    }
}

void TransitionHandleNode::set_transition_duration(float p_duration) {
    transition_duration = CLAMP(p_duration, 0.0f, max_duration);
    queue_redraw();
}

float TransitionHandleNode::get_transition_duration() const {
    return transition_duration;
}

void TransitionHandleNode::set_max_duration(float p_max) {
    max_duration = p_max;
    if (transition_duration > max_duration) {
        set_transition_duration(max_duration);
    }
}

float TransitionHandleNode::get_max_duration() const {
    return max_duration;
}

void TransitionHandleNode::set_handle_color(const Color &p_color) {
    handle_color = p_color;
    queue_redraw();
}

Color TransitionHandleNode::get_handle_color() const {
    return handle_color;
}