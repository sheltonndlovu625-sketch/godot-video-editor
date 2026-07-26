#include "timeline_clip_container.h"
#include <godot_cpp/variant/utility_functions.hpp>

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
    for (int i = 0; i < clips.size(); i++) {
        Ref<TimelineClip> clip = clips[i];
        if (clip.is_null()) continue;

        TimelineClipNode *node = memnew(TimelineClipNode);
        node->set_clip(clip);
        node->set_pixels_per_second(pixels_per_second);
        node->set_zoom(zoom);
        node->set_is_video(track->get_track_type() == TimelineTrack::TRACK_TYPE_VIDEO);

        // Optional: connect signals if you want the track to re-sort after a move
        // node->connect("moved", callable_mp(this, &TimelineClipContainer::_on_clip_moved));

        add_child(node, false, INTERNAL_MODE_DISABLED);
    }
}

void TimelineClipContainer::_update_layout() {
    if (track.is_null()) return;

    float pps = pixels_per_second * zoom;

    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++) {
        TimelineClipNode *node = Object::cast_to<TimelineClipNode>(children[i]);
        if (!node) continue;

        Ref<TimelineClip> clip = node->get_clip();
        if (clip.is_null()) continue;

        // Position based on model data. The node may override this during drag,
        // which is fine — we only call _update_layout() on zoom/PPS changes.
        float x = header_width + (float)(clip->get_timeline_start() * pps);
        Vector2 pos = node->get_position();
        pos.x = x;
        node->set_position(pos);

        node->set_pixels_per_second(pixels_per_second);
        node->set_zoom(zoom);
        node->update_layout();
    }
}
