// timeline_zoom_controller.cpp
#include "timeline_zoom_controller.h"
#include "timeline_track_node.h"
#include "timeline_ruler.h"
#include "timeline_playhead.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>

using namespace godot;

void TimelineZoomController::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineZoomController::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineZoomController::get_zoom);
    ClassDB::add_property("TimelineZoomController", PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

    ClassDB::bind_method(D_METHOD("set_min_zoom", "min"), &TimelineZoomController::set_min_zoom);
    ClassDB::bind_method(D_METHOD("get_min_zoom"), &TimelineZoomController::get_min_zoom);
    ClassDB::add_property("TimelineZoomController", PropertyInfo(Variant::FLOAT, "min_zoom"), "set_min_zoom", "get_min_zoom");

    ClassDB::bind_method(D_METHOD("set_max_zoom", "max"), &TimelineZoomController::set_max_zoom);
    ClassDB::bind_method(D_METHOD("get_max_zoom"), &TimelineZoomController::get_max_zoom);
    ClassDB::add_property("TimelineZoomController", PropertyInfo(Variant::FLOAT, "max_zoom"), "set_max_zoom", "get_max_zoom");

    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineZoomController::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineZoomController::get_pixels_per_second);
    ClassDB::add_property("TimelineZoomController", PropertyInfo(Variant::FLOAT, "pixels_per_second"), "set_pixels_per_second", "get_pixels_per_second");

    ClassDB::bind_method(D_METHOD("zoom_in"), &TimelineZoomController::zoom_in);
    ClassDB::bind_method(D_METHOD("zoom_out"), &TimelineZoomController::zoom_out);
    ClassDB::bind_method(D_METHOD("apply_zoom_to_children"), &TimelineZoomController::apply_zoom_to_children);

    ADD_SIGNAL(MethodInfo("zoom_changed", PropertyInfo(Variant::FLOAT, "zoom")));
}

TimelineZoomController::TimelineZoomController() {}

void TimelineZoomController::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        set_process_input(true);
        apply_zoom_to_children();
    }
}

void TimelineZoomController::set_zoom(float p_zoom) {
    float new_zoom = CLAMP(p_zoom, min_zoom, max_zoom);
    if (zoom != new_zoom) {
        zoom = new_zoom;
        apply_zoom_to_children();
        emit_signal("zoom_changed", zoom);
    }
}
float TimelineZoomController::get_zoom() const { return zoom; }

void TimelineZoomController::set_min_zoom(float p_min) { min_zoom = Math::max(0.01f, p_min); }
float TimelineZoomController::get_min_zoom() const { return min_zoom; }

void TimelineZoomController::set_max_zoom(float p_max) { max_zoom = Math::max(min_zoom, p_max); }
float TimelineZoomController::get_max_zoom() const { return max_zoom; }

void TimelineZoomController::set_pixels_per_second(float p_pps) {
    pixels_per_second = Math::max(1.0f, p_pps);
    apply_zoom_to_children();
}
float TimelineZoomController::get_pixels_per_second() const { return pixels_per_second; }

void TimelineZoomController::zoom_in() { set_zoom(zoom * 1.2f); }
void TimelineZoomController::zoom_out() { set_zoom(zoom / 1.2f); }

void TimelineZoomController::apply_zoom_to_children() {
    Node *parent = get_parent();
    if (!parent) return;

    for (int i = 0; i < parent->get_child_count(); i++) {
        Node *child = parent->get_child(i);

        TimelineTrackNode *track = Object::cast_to<TimelineTrackNode>(child);
        if (track) {
            track->set_zoom(zoom);
            track->set_pixels_per_second(pixels_per_second);
            continue;
        }
        TimelineRuler *ruler = Object::cast_to<TimelineRuler>(child);
        if (ruler) {
            ruler->set_zoom(zoom);
            ruler->set_pixels_per_second(pixels_per_second);
            continue;
        }
        TimelinePlayhead *playhead = Object::cast_to<TimelinePlayhead>(child);
        if (playhead) {
            playhead->set_zoom(zoom);
            playhead->set_pixels_per_second(pixels_per_second);
            continue;
        }
    }
}

void TimelineZoomController::_input(const Ref<InputEvent> &p_event) {
    // 1. Native magnify gesture (pinch on mobile / trackpad on desktop)
    Ref<InputEventMagnifyGesture> magnify = p_event;
    if (magnify.is_valid()) {
        set_zoom(zoom * magnify->get_factor());
        return;
    }

    // 2. Manual two-finger pinch fallback
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid()) {
        int idx = touch->get_index();
        if (touch->is_pressed()) {
            touches[idx] = touch->get_position();
            if (touches.size() == 2) {
                Array keys = touches.keys();
                Vector2 p0 = touches[keys[0]];
                Vector2 p1 = touches[keys[1]];
                last_pinch_distance = p0.distance_to(p1);
                last_pinch_zoom = zoom;
            }
        } else {
            touches.erase(idx);
            if (touches.size() < 2) {
                last_pinch_distance = -1.0f;
            }
        }
        return;
    }

    Ref<InputEventScreenDrag> drag = p_event;
    if (drag.is_valid() && touches.size() == 2) {
        int idx = drag->get_index();
        if (touches.has(idx)) {
            touches[idx] = drag->get_position();
            Array keys = touches.keys();
            if (keys.size() == 2) {
                Vector2 p0 = touches[keys[0]];
                Vector2 p1 = touches[keys[1]];
                float dist = p0.distance_to(p1);
                if (last_pinch_distance > 0.0f && dist > 0.0f) {
                    float ratio = dist / last_pinch_distance;
                    set_zoom(last_pinch_zoom * ratio);
                }
            }
        }
        return;
    }
}
