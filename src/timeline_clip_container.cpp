#include "timeline_clip_container.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

void TimelineClipContainer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_track", "track"), &TimelineClipContainer::set_track);
    ClassDB::bind_method(D_METHOD("get_track"), &TimelineClipContainer::get_track);

    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineClipContainer::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineClipContainer::get_pixels_per_second);

    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineClipContainer::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineClipContainer::get_zoom);

    ClassDB::bind_method(D_METHOD("set_header_width", "width"), &TimelineClipContainer::set_header_width);
    ClassDB::bind_method(D_METHOD("get_header_width"), &TimelineClipContainer::get_header_width);

    ClassDB::bind_method(D_METHOD("refresh"), &TimelineClipContainer::refresh);
}

TimelineClipContainer::TimelineClipContainer() {}

void TimelineClipContainer::_notification(int p_what) {
    if (p_what == NOTIFICATION_RESIZED) {
        _update_layout();
    }
}

void TimelineClipContainer::set_track(const Ref<TimelineTrack> &p_track) {
    if (track == p_track) return;
    track = p_track;
    refresh();
}

Ref<TimelineTrack> TimelineClipContainer::get_track() const {
    return track;
}

void TimelineClipContainer::set_pixels_per_second(float p_pps) {
    pixels_per_second = p_pps;
    _update_layout();
}

float TimelineClipContainer::get_pixels_per_second() const {
    return pixels_per_second;
}

void TimelineClipContainer::set_zoom(float p_zoom) {
    zoom = p_zoom;
    _update_layout();
}

float TimelineClipContainer::get_zoom() const {
    return zoom;
}

void TimelineClipContainer::set_header_width(float p_width) {
    header_width = p_width;
    _update_layout();
}

float TimelineClipContainer::get_header_width() const {
    return header_width;
}

void TimelineClipContainer::refresh() {
    _clear_clip_nodes();
    if (track.is_valid()) {
        _create_clip_nodes();
    }
    _update_layout();
}

void TimelineClipContainer::_clear_clip_nodes() {
    TypedArray<Node> children = get_children();
    for (int i = children.size() - 1; i >= 0; i--) {
        TimelineClipNode *node = Object::cast_to<TimelineClipNode>(children[i]);
        if (node) {
            remove_child(node);
            node->queue_free();
        }
    }
}

void TimelineClipContainer::_create_clip_nodes() {
    if (track.is_null()) return;

    TypedArray<TimelineClip> clips = track->get_clips();
    float h = get_size().y;

    for (int i = 0; i < clips.size(); i++) {
        Ref<TimelineClip> clip = clips[i];
        if (clip.is_null()) continue;

        TimelineClipNode *node = memnew(TimelineClipNode);
        node->set_clip(clip);
        node->set_pixels_per_second(pixels_per_second);
        node->set_zoom(zoom);
        node->set_is_video(track->get_track_type() == TimelineTrack::TRACK_TYPE_VIDEO);

        // CRITICAL: initialise height so the node is actually visible
        node->set_custom_minimum_size(Vector2(24.0f, h));
        node->set_size(Vector2(100.0f, h));   // width is recomputed by update_layout()

        add_child(node, false, INTERNAL_MODE_DISABLED);
    }
}

void TimelineClipContainer::_update_layout() {
    if (track.is_null()) return;

    float pps = pixels_per_second * zoom;
    float h = get_size().y;

    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++) {
        TimelineClipNode *node = Object::cast_to<TimelineClipNode>(children[i]);
        if (!node) continue;

        Ref<TimelineClip> clip = node->get_clip();
        if (clip.is_null()) continue;

        // Position from model data. During drag the node overrides this;
        // we only snap back to truth when zoom/PPS changes or refresh() is called.
        float x = header_width + (float)(clip->get_timeline_start() * pps);
        Vector2 pos = node->get_position();
        pos.x = x;
        node->set_position(pos);

        node->set_pixels_per_second(pixels_per_second);
        node->set_zoom(zoom);
        node->update_layout();

        // Keep height locked to container so _draw() always has a non-empty rect
        node->set_size(Vector2(node->get_size().x, h));
        node->set_custom_minimum_size(Vector2(node->get_custom_minimum_size().x, h));
    }
}
