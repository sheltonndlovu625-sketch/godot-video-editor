#include "transition_handle_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>

using namespace godot;

void TransitionHandleNode::_bind_methods() {
    // Duration
    ClassDB::bind_method(D_METHOD("set_transition_duration", "duration"), &TransitionHandleNode::set_transition_duration);
    ClassDB::bind_method(D_METHOD("get_transition_duration"), &TransitionHandleNode::get_transition_duration);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "transition_duration", PROPERTY_HINT_RANGE, "0.0,60.0,0.01"), "set_transition_duration", "get_transition_duration");

    ClassDB::bind_method(D_METHOD("set_max_duration", "max_duration"), &TransitionHandleNode::set_max_duration);
    ClassDB::bind_method(D_METHOD("get_max_duration"), &TransitionHandleNode::get_max_duration);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "max_duration", PROPERTY_HINT_RANGE, "0.0,60.0,0.01"), "set_max_duration", "get_max_duration");

    // Visuals
    ClassDB::bind_method(D_METHOD("set_handle_color", "color"), &TransitionHandleNode::set_handle_color);
    ClassDB::bind_method(D_METHOD("get_handle_color"), &TransitionHandleNode::get_handle_color);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::COLOR, "handle_color"), "set_handle_color", "get_handle_color");

    ClassDB::bind_method(D_METHOD("set_texture_path", "path"), &TransitionHandleNode::set_texture_path);
    ClassDB::bind_method(D_METHOD("get_texture_path"), &TransitionHandleNode::get_texture_path);
    ClassDB::add_property(
        "TransitionHandleNode",
        PropertyInfo(Variant::STRING, "texture_path", PROPERTY_HINT_FILE, "*.png,*.jpg,*.jpeg,*.webp,*.tga,*.bmp"),
        "set_texture_path",
        "get_texture_path"
    );

    // Zoom visibility
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TransitionHandleNode::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TransitionHandleNode::get_zoom);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

    ClassDB::bind_method(D_METHOD("set_zoom_threshold", "threshold"), &TransitionHandleNode::set_zoom_threshold);
    ClassDB::bind_method(D_METHOD("get_zoom_threshold"), &TransitionHandleNode::get_zoom_threshold);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::FLOAT, "zoom_threshold", PROPERTY_HINT_RANGE, "0.1,20.0,0.1"), "set_zoom_threshold", "get_zoom_threshold");

    // Selection
    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &TransitionHandleNode::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &TransitionHandleNode::is_selected);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::BOOL, "selected"), "set_selected", "is_selected");

    // Effect resource
    ClassDB::bind_method(D_METHOD("set_transition_effect", "effect"), &TransitionHandleNode::set_transition_effect);
    ClassDB::bind_method(D_METHOD("get_transition_effect"), &TransitionHandleNode::get_transition_effect);
    ClassDB::add_property("TransitionHandleNode", PropertyInfo(Variant::OBJECT, "transition_effect", PROPERTY_HINT_RESOURCE_TYPE, "TransitionEffect"), "set_transition_effect", "get_transition_effect");

    // Signals
    ADD_SIGNAL(MethodInfo("pressed"));
    ADD_SIGNAL(MethodInfo("selected"));
}

TransitionHandleNode::TransitionHandleNode() {
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_default_cursor_shape(CURSOR_POINTING_HAND);
    set_custom_minimum_size(Vector2(24, 24));
}

TransitionHandleNode::~TransitionHandleNode() {}

void TransitionHandleNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();

        // Thumbnail or fallback switch
        if (cached_texture.is_valid()) {
            draw_texture_rect(cached_texture, Rect2(Vector2(), size), false);
        } else {
            draw_rect(Rect2(Vector2(), size), handle_color, true);

            // Grip lines
            float center_x = size.x / 2.0f;
            draw_line(Vector2(center_x - 2, size.y * 0.3f), Vector2(center_x - 2, size.y * 0.7f), Color(1.0, 1.0, 1.0, 0.5), 1.0);
            draw_line(Vector2(center_x + 2, size.y * 0.3f), Vector2(center_x + 2, size.y * 0.7f), Color(1.0, 1.0, 1.0, 0.5), 1.0);
        }

        // Outline
        draw_rect(Rect2(Vector2(), size), Color(1.0, 1.0, 1.0, 0.9), false, 1.0);

        // Selection highlight
        if (selected) {
            draw_rect(Rect2(Vector2(), size), Color(1.0, 0.85, 0.2, 0.6), false, 2.0);
        }
    }
}

void TransitionHandleNode::_gui_input(const Ref<InputEvent> &p_event) {
    // Mobile touch
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            _handle_press(touch->get_position());
        }
        return;
    }

    // Desktop mouse
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (mb->is_pressed()) {
            _handle_press(mb->get_position());
        }
        return;
    }
}

void TransitionHandleNode::_handle_press(const Vector2 &p_pos) {
    set_selected(true);
    emit_signal("pressed");
    emit_signal("selected");
}

// ---- Duration ----

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
    queue_redraw();
}

float TransitionHandleNode::get_max_duration() const {
    return max_duration;
}

// ---- Visuals ----

void TransitionHandleNode::set_handle_color(const Color &p_color) {
    handle_color = p_color;
    queue_redraw();
}

Color TransitionHandleNode::get_handle_color() const {
    return handle_color;
}

void TransitionHandleNode::set_texture_path(const String &p_path) {
    if (texture_path == p_path) return;

    texture_path = p_path;
    image_loaded = false;
    cached_image.unref();
    cached_texture.unref();

    if (!texture_path.is_empty()) {
        String resolved = texture_path;
        if (resolved.begins_with("user://") || resolved.begins_with("res://")) {
            ProjectSettings *ps = ProjectSettings::get_singleton();
            if (ps) resolved = ps->globalize_path(resolved);
        }

        cached_image = Image::load_from_file(resolved);
        if (cached_image.is_valid()) {
            cached_texture.instantiate();
            cached_texture->set_image(cached_image);
            image_loaded = true;
        } else {
            UtilityFunctions::push_error("[TransitionHandleNode] Failed to load image: ", texture_path);
        }
    }

    queue_redraw();
}

String TransitionHandleNode::get_texture_path() const {
    return texture_path;
}

// ---- Zoom visibility ----

void TransitionHandleNode::set_zoom(float p_zoom) {
    zoom = MAX(0.01f, p_zoom);
    bool should_show = zoom <= zoom_threshold;
    if (should_show != is_visible()) {
        set_visible(should_show);
    }
    queue_redraw();
}

float TransitionHandleNode::get_zoom() const {
    return zoom;
}

void TransitionHandleNode::set_zoom_threshold(float p_threshold) {
    zoom_threshold = MAX(0.1f, p_threshold);
    // Re-evaluate visibility immediately
    set_zoom(zoom);
    queue_redraw();
}

float TransitionHandleNode::get_zoom_threshold() const {
    return zoom_threshold;
}

// ---- Selection ----

void TransitionHandleNode::set_selected(bool p_selected) {
    if (selected == p_selected) return;
    selected = p_selected;
    queue_redraw();
}

bool TransitionHandleNode::is_selected() const {
    return selected;
}

// ---- Effect ----

void TransitionHandleNode::set_transition_effect(const Ref<TransitionEffect> &p_effect) {
    transition_effect = p_effect;
}

Ref<TransitionEffect> TransitionHandleNode::get_transition_effect() const {
    return transition_effect;
}
