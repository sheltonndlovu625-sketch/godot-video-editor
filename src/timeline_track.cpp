#include "timeline_track.h"

using namespace godot;

void TimelineTrack::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_track_type", "type"), &TimelineTrack::set_track_type);
    ClassDB::bind_method(D_METHOD("get_track_type"), &TimelineTrack::get_track_type);
    ClassDB::add_property("TimelineTrack", PropertyInfo(Variant::INT, "track_type"), "set_track_type", "get_track_type");

    ClassDB::bind_method(D_METHOD("set_layer_index", "index"), &TimelineTrack::set_layer_index);
    ClassDB::bind_method(D_METHOD("get_layer_index"), &TimelineTrack::get_layer_index);
    ClassDB::add_property("TimelineTrack", PropertyInfo(Variant::INT, "layer_index"), "set_layer_index", "get_layer_index");

    ClassDB::bind_method(D_METHOD("add_clip", "clip"), &TimelineTrack::add_clip);
    ClassDB::bind_method(D_METHOD("remove_clip", "index"), &TimelineTrack::remove_clip);
    ClassDB::bind_method(D_METHOD("clear_clips"), &TimelineTrack::clear_clips);
    ClassDB::bind_method(D_METHOD("get_clip_count"), &TimelineTrack::get_clip_count);
    ClassDB::bind_method(D_METHOD("get_clip", "index"), &TimelineTrack::get_clip);
    ClassDB::bind_method(D_METHOD("get_clip_at_time", "time"), &TimelineTrack::get_clip_at_time);
    ClassDB::bind_method(D_METHOD("get_clips"), &TimelineTrack::get_clips);
    ClassDB::bind_method(D_METHOD("get_track_duration"), &TimelineTrack::get_track_duration);

    BIND_ENUM_CONSTANT(TRACK_TYPE_VIDEO);
    BIND_ENUM_CONSTANT(TRACK_TYPE_AUDIO);
}

TimelineTrack::TimelineTrack() {}

TimelineTrack::~TimelineTrack() {}

void TimelineTrack::set_track_type(int p_type) {
    track_type = (TimelineTrackType)p_type;
}

int TimelineTrack::get_track_type() const {
    return (int)track_type;
}

void TimelineTrack::set_layer_index(int p_index) {
    layer_index = p_index;
}

int TimelineTrack::get_layer_index() const {
    return layer_index;
}

void TimelineTrack::add_clip(const Ref<TimelineClip> &p_clip) {
    if (p_clip.is_null()) return;
    clips.push_back(p_clip);
    // Keep clips sorted by timeline_start
    clips.sort_custom<Ref<TimelineClip>>([](const Ref<TimelineClip> &a, const Ref<TimelineClip> &b) {
        return a->get_timeline_start() < b->get_timeline_start();
    });
}

void TimelineTrack::remove_clip(int p_index) {
    if (p_index >= 0 && p_index < clips.size()) {
        clips.remove_at(p_index);
    }
}

void TimelineTrack::clear_clips() {
    clips.clear();
}

int TimelineTrack::get_clip_count() const {
    return clips.size();
}

Ref<TimelineClip> TimelineTrack::get_clip(int p_index) const {
    if (p_index >= 0 && p_index < clips.size()) {
        return clips[p_index];
    }
    return Ref<TimelineClip>();
}

Ref<TimelineClip> TimelineTrack::get_clip_at_time(double p_time) const {
    for (int i = 0; i < clips.size(); i++) {
        const Ref<TimelineClip> &clip = clips[i];
        if (p_time >= clip->get_timeline_start() && p_time < clip->get_timeline_end()) {
            return clip;
        }
    }
    return Ref<TimelineClip>();
}

TypedArray<TimelineClip> TimelineTrack::get_clips() const {
    TypedArray<TimelineClip> result;
    for (int i = 0; i < clips.size(); i++) {
        result.push_back(clips[i]);
    }
    return result;
}

double TimelineTrack::get_track_duration() const {
    double max_end = 0.0;
    for (int i = 0; i < clips.size(); i++) {
        double end = clips[i]->get_timeline_end();
        if (end > max_end) {
            max_end = end;
        }
    }
    return max_end;
}

