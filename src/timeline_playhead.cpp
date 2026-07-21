// timeline_playhead.cpp
#include "timeline_playhead.h"
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/font.hpp>   // <-- ADD THIS LINE
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimelinePlayhead::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_time", "time"), &TimelinePlayhead::set_time);
    ClassDB::bind_method(D_METHOD("get_time"), &TimelinePlayhead::get_time);
    ClassDB::add_property("TimelinePlayhead", PropertyInfo(Variant::FLOAT, "time"), "set_time", "get_time");

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &TimelinePlayhead::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &TimelinePlayhead::get_duration);
    ClassDB::add_property("TimelinePlayhead", PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");

    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelinePlayhead::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelinePlayhead::get_pixels_per_second);
    ClassDB::add_property("TimelinePlayhead", PropertyInfo(Variant::FLOAT, "pixels_per_second"), "set_pixels_per_second", "get_pixels_per_second");

    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelinePlayhead::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelinePlayhead::get_zoom);
    ClassDB::add_property("TimelinePlayhead", PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

    ClassDB::bind_method(D_METHOD("set_header_width", "width"), &TimelinePlayhead::set_header_width);
    ClassDB::bind_method(D_METHOD("get_header_width"), &TimelinePlayhead::get_header_width);
    ClassDB::add_property("TimelinePlayhead", PropertyInfo(Variant::FLOAT, "header_width"), "set_header_width", "get_header_width");

    ADD_SIGNAL(MethodInfo("time_changed", PropertyInfo(Variant::FLOAT, "time")));
    ADD_SIGNAL(MethodInfo("seek_started"));
    ADD_SIGNAL(MethodInfo("seek_ended"));
}

TimelinePlayhead::TimelinePlayhead() {
    set_custom_minimum_size(Vector2(20, 40));
}

void TimelinePlayhead::set_time(double p_time) {
    p_time = CLAMP(p_time, 0.0, duration);
    if (current_time != p_time) {
        current_time = p_time;
        queue_redraw();
    }
}
double TimelinePlayhead::get_time() const { return current_time; }

void TimelinePlayhead::set_duration(double p_duration) {
    duration = Math::max(0.0, p_duration);
    queue_redraw();
}
double TimelinePlayhead::get_duration() const { return duration; }

void TimelinePlayhead::set_pixels_per_second(float p_pps) {
    pixels_per_second = Math::max(1.0f, p_pps);
    queue_redraw();
}
float TimelinePlayhead::get_pixels_per_second() const { return pixels_per_second; }

void TimelinePlayhead::set_zoom(float p_zoom) {
    zoom = CLAMP(p_zoom, 0.01f, 100.0f);
    queue_redraw();
}
float TimelinePlayhead::get_zoom() const { return zoom; }

void TimelinePlayhead::set_header_width(float p_width) {
    header_width = Math::max(0.0f, p_width);
    queue_redraw();
}
float TimelinePlayhead::get_header_width() const { return header_width; }

void TimelinePlayhead::_draw() {
    float h = get_size().y;
    float pps = pixels_per_second * zoom;
    float x = header_width + current_time * pps;

    draw_line(Vector2(x, 0), Vector2(x, h), line_color, 2.0f);

    PackedVector2Array tri;
    tri.push_back(Vector2(x, 0));
    tri.push_back(Vector2(x - 6, -10));
    tri.push_back(Vector2(x + 6, -10));
    draw_colored_polygon(tri, line_color);

    int mins = int(current_time) / 60;
    int secs = int(current_time) % 60;
    int frames = int((current_time - int(current_time)) * 30.0);
    String label = String::num_int64(mins).pad_zeros(2) + ":" +
                   String::num_int64(secs).pad_zeros(2) + ":" +
                   String::num_int64(frames).pad_zeros(2);
    draw_string(get_theme_default_font(), Vector2(x + 8, -2), label,
                HORIZONTAL_ALIGNMENT_LEFT, -1, 12, line_color);
}

void TimelinePlayhead::_gui_input(const Ref<InputEvent> &p_event) {
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            Vector2 pos = touch->get_position();
            float pps = pixels_per_second * zoom;
            float x = header_width + current_time * pps;
            if (Math::abs(pos.x - x) < 20.0f) {
                dragging = true;
                emit_signal("seek_started");
            }
        } else {
            if (dragging) {
                dragging = false;
                emit_signal("seek_ended");
            }
        }
        return;
    }

    Ref<InputEventScreenDrag> drag = p_event;
    if (drag.is_valid() && dragging && drag->get_index() == 0) {
        float pps = pixels_per_second * zoom;
        double new_time = (drag->get_position().x - header_width) / pps;
        set_time(new_time);
        emit_signal("time_changed", current_time);
        return;
    }

    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (mb->is_pressed()) {
            Vector2 pos = mb->get_position();
            float pps = pixels_per_second * zoom;
            float x = header_width + current_time * pps;
            if (Math::abs(pos.x - x) < 20.0f) {
                dragging = true;
                emit_signal("seek_started");
            }
        } else {
            if (dragging) {
                dragging = false;
                emit_signal("seek_ended");
            }
        }
        return;
    }

    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && dragging) {
        float pps = pixels_per_second * zoom;
        double new_time = (mm->get_position().x - header_width) / pps;
        set_time(new_time);
        emit_signal("time_changed", current_time);
    }
}
