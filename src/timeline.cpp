#include "timeline.h"

using namespace godot;

void Timeline::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_track", "track"), &Timeline::add_track);
    ClassDB::bind_method(D_METHOD("remove_track", "index"), &Timeline::remove_track);
    ClassDB::bind_method(D_METHOD("clear_tracks"), &Timeline::clear_tracks);
    ClassDB::bind_method(D_METHOD("get_track_count"), &Timeline::get_track_count);
    ClassDB::bind_method(D_METHOD("get_track", "index"), &Timeline::get_track);

    ClassDB::bind_method(D_METHOD("get_video_tracks"), &Timeline::get_video_tracks);
    ClassDB::bind_method(D_METHOD("get_audio_tracks"), &Timeline::get_audio_tracks);

    ClassDB::bind_method(D_METHOD("set_playhead_time", "time"), &Timeline::set_playhead_time);
    ClassDB::bind_method(D_METHOD("get_playhead_time"), &Timeline::get_playhead_time);
    ClassDB::add_property("Timeline", PropertyInfo(Variant::FLOAT, "playhead_time"), "set_playhead_time", "get_playhead_time");

    ClassDB::bind_method(D_METHOD("set_frame_rate", "fps"), &Timeline::set_frame_rate);
    ClassDB::bind_method(D_METHOD("get_frame_rate"), &Timeline::get_frame_rate);
    ClassDB::add_property("Timeline", PropertyInfo(Variant::FLOAT, "frame_rate"), "set_frame_rate", "get_frame_rate");

    ClassDB::bind_method(D_METHOD("get_duration"), &Timeline::get_duration);
    ClassDB::bind_method(D_METHOD("set_playhead_frame", "frame"), &Timeline::set_playhead_frame);
    ClassDB::bind_method(D_METHOD("get_playhead_frame"), &Timeline::get_playhead_frame);
    ClassDB::bind_method(D_METHOD("step_forward"), &Timeline::step_forward);
    ClassDB::bind_method(D_METHOD("step_backward"), &Timeline::step_backward);
    ClassDB::bind_method(D_METHOD("is_at_end"), &Timeline::is_at_end);

    // Text overlay bindings
    ClassDB::bind_method(D_METHOD("add_text_overlay", "overlay"), &Timeline::add_text_overlay);
    ClassDB::bind_method(D_METHOD("remove_text_overlay", "index"), &Timeline::remove_text_overlay);
    ClassDB::bind_method(D_METHOD("clear_text_overlays"), &Timeline::clear_text_overlays);
    ClassDB::bind_method(D_METHOD("get_text_overlay_count"), &Timeline::get_text_overlay_count);
    ClassDB::bind_method(D_METHOD("get_text_overlay", "index"), &Timeline::get_text_overlay);
    ClassDB::bind_method(D_METHOD("get_text_overlays_at_time", "time"), &Timeline::get_text_overlays_at_time);
    ClassDB::bind_method(D_METHOD("get_all_text_overlays"), &Timeline::get_all_text_overlays);
}

Timeline::Timeline() {}
Timeline::~Timeline() {}

void Timeline::add_track(const Ref<TimelineTrack> &p_track) {
    if (p_track.is_null()) return;
    tracks.push_back(p_track);
}

void Timeline::remove_track(int p_index) {
    if (p_index >= 0 && p_index < tracks.size()) {
        tracks.remove_at(p_index);
    }
}

void Timeline::clear_tracks() {
    tracks.clear();
}

int Timeline::get_track_count() const {
    return tracks.size();
}

Ref<TimelineTrack> Timeline::get_track(int p_index) const {
    if (p_index >= 0 && p_index < tracks.size()) {
        return tracks[p_index];
    }
    return Ref<TimelineTrack>();
}

TypedArray<TimelineTrack> Timeline::get_video_tracks() const {
    TypedArray<TimelineTrack> result;
    for (int i = 0; i < tracks.size(); i++) {
        if (tracks[i]->get_track_type() == TimelineTrack::TRACK_TYPE_VIDEO) {
            result.push_back(tracks[i]);
        }
    }
    return result;
}

TypedArray<TimelineTrack> Timeline::get_audio_tracks() const {
    TypedArray<TimelineTrack> result;
    for (int i = 0; i < tracks.size(); i++) {
        if (tracks[i]->get_track_type() == TimelineTrack::TRACK_TYPE_AUDIO) {
            result.push_back(tracks[i]);
        }
    }
    return result;
}

void Timeline::set_playhead_time(double p_time) {
    playhead_time = p_time;
    if (playhead_time < 0.0) {
        playhead_time = 0.0;
    }
}

double Timeline::get_playhead_time() const {
    return playhead_time;
}

void Timeline::set_frame_rate(double p_fps) {
    if (p_fps > 0.0) {
        frame_rate = p_fps;
    }
}

double Timeline::get_frame_rate() const {
    return frame_rate;
}

double Timeline::get_duration() const {
    double max_duration = 0.0;
    for (int i = 0; i < tracks.size(); i++) {
        double track_dur = tracks[i]->get_track_duration();
        if (track_dur > max_duration) {
            max_duration = track_dur;
        }
    }
    // Also check text overlays
    for (int i = 0; i < text_overlays.size(); i++) {
        Ref<TextOverlay> ov = text_overlays[i];
        if (ov.is_valid() && ov->get_end_time() > max_duration) {
            max_duration = ov->get_end_time();
        }
    }
    return max_duration;
}

void Timeline::set_playhead_frame(int p_frame) {
    playhead_time = double(p_frame) / frame_rate;
    if (playhead_time < 0.0) {
        playhead_time = 0.0;
    }
}

int Timeline::get_playhead_frame() const {
    return int(playhead_time * frame_rate);
}

void Timeline::step_forward() {
    playhead_time += 1.0 / frame_rate;
}

void Timeline::step_backward() {
    playhead_time -= 1.0 / frame_rate;
    if (playhead_time < 0.0) {
        playhead_time = 0.0;
    }
}

bool Timeline::is_at_end() const {
    return playhead_time >= get_duration();
}

// ---- Text overlay implementations ----

void Timeline::add_text_overlay(const Ref<TextOverlay> &p_overlay) {
    if (p_overlay.is_valid()) {
        text_overlays.push_back(p_overlay);
    }
}

void Timeline::remove_text_overlay(int p_index) {
    if (p_index >= 0 && p_index < text_overlays.size()) {
        text_overlays.remove_at(p_index);
    }
}

void Timeline::clear_text_overlays() {
    text_overlays.clear();
}

int Timeline::get_text_overlay_count() const {
    return text_overlays.size();
}

Ref<TextOverlay> Timeline::get_text_overlay(int p_index) const {
    if (p_index >= 0 && p_index < text_overlays.size()) {
        return text_overlays[p_index];
    }
    return Ref<TextOverlay>();
}

TypedArray<TextOverlay> Timeline::get_text_overlays_at_time(double p_time) const {
    TypedArray<TextOverlay> result;
    for (int i = 0; i < text_overlays.size(); i++) {
        Ref<TextOverlay> ov = text_overlays[i];
        if (ov.is_valid() && ov->is_visible_at(p_time)) {
            result.push_back(ov);
        }
    }
    return result;
}

TypedArray<TextOverlay> Timeline::get_all_text_overlays() const {
    return text_overlays;
}
