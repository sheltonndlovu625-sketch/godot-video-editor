#include "timeline_track_node.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>


using namespace godot;

void TimelineTrackNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_track", "track"), &TimelineTrackNode::set_track);
    ClassDB::bind_method(D_METHOD("get_track"), &TimelineTrackNode::get_track);
    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineTrackNode::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineTrackNode::get_pixels_per_second);
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineTrackNode::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineTrackNode::get_zoom);
    ClassDB::bind_method(D_METHOD("set_header_width", "width"), &TimelineTrackNode::set_header_width);
    ClassDB::bind_method(D_METHOD("get_header_width"), &TimelineTrackNode::get_header_width);

    ADD_SIGNAL(MethodInfo("seek_requested", PropertyInfo(Variant::FLOAT, "time")));
}

TimelineTrackNode::TimelineTrackNode() {}

void TimelineTrackNode::set_track(const Ref<TimelineTrack> &p_track) {
    track = p_track;
    if (track.is_valid()) {
        is_video_track = track->get_track_type() == TimelineTrack::TRACK_TYPE_VIDEO;
    }
    queue_redraw();
}

Ref<TimelineTrack> TimelineTrackNode::get_track() const {
    return track;
}

void TimelineTrackNode::set_pixels_per_second(float p_pps) {
    pixels_per_second = p_pps;
}

float TimelineTrackNode::get_pixels_per_second() const {
    return pixels_per_second;
}

void TimelineTrackNode::set_zoom(float p_zoom) {
    zoom = p_zoom;
}

float TimelineTrackNode::get_zoom() const {
    return zoom;
}

void TimelineTrackNode::set_header_width(float p_width) {
    header_width = p_width;
    queue_redraw();
}

float TimelineTrackNode::get_header_width() const {
    return header_width;
}

void TimelineTrackNode::_draw() {
    float h = get_size().y;

    draw_rect(Rect2(0, 0, header_width, h), Color(0.12f, 0.12f, 0.14f));
    if (track.is_valid()) {
        String icon = is_video_track ? "V" : "A";
        draw_string(get_theme_default_font(), Vector2(6, h * 0.5f + 5), icon, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(1, 1, 1));
    }

    Color bg = is_video_track ? Color(0.05f, 0.05f, 0.07f) : Color(0.06f, 0.06f, 0.08f);
    draw_rect(Rect2(header_width, 0, get_size().x - header_width, h), bg);

    float pps = pixels_per_second * zoom;
    for (float x = header_width; x < get_size().x; x += pps) {
        draw_line(Vector2(x, 0), Vector2(x, h), Color(1, 1, 1, 0.03f), 1.0f);
    }
}

void TimelineTrackNode::_gui_input(const Ref<InputEvent> &p_event) {
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->is_pressed() && touch->get_index() == 0) {
        Vector2 pos = touch->get_position();
        if (pos.x > header_width) {
            double t = (pos.x - header_width) / (pixels_per_second * zoom);
            // FIX: Math::max instead of maxf
            emit_signal("seek_requested", Math::max(0.0f, (float)t));
        }
    }
}
