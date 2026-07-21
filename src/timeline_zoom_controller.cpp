// timeline_zoom_controller.cpp
#include "timeline_zoom_controller.h"
#include "timeline_track_node.h"
#include "timeline_ruler.h"
#include "timeline_playhead.h"
#include <godot_cpp/variant/utility_functions.hpp>

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
