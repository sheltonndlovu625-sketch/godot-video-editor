#include "timeline_clip.h"

using namespace godot;

void TimelineClip::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_source_path", "path"), &TimelineClip::set_source_path);
    ClassDB::bind_method(D_METHOD("get_source_path"), &TimelineClip::get_source_path);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::STRING, "source_path"), "set_source_path", "get_source_path");

    ClassDB::bind_method(D_METHOD("set_timeline_start", "time"), &TimelineClip::set_timeline_start);
    ClassDB::bind_method(D_METHOD("get_timeline_start"), &TimelineClip::get_timeline_start);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "timeline_start"), "set_timeline_start", "get_timeline_start");

    ClassDB::bind_method(D_METHOD("set_source_in_point", "time"), &TimelineClip::set_source_in_point);
    ClassDB::bind_method(D_METHOD("get_source_in_point"), &TimelineClip::get_source_in_point);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "source_in_point"), "set_source_in_point", "get_source_in_point");

    ClassDB::bind_method(D_METHOD("set_source_out_point", "time"), &TimelineClip::set_source_out_point);
    ClassDB::bind_method(D_METHOD("get_source_out_point"), &TimelineClip::get_source_out_point);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "source_out_point"), "set_source_out_point", "get_source_out_point");

    ClassDB::bind_method(D_METHOD("set_playback_speed", "speed"), &TimelineClip::set_playback_speed);
    ClassDB::bind_method(D_METHOD("get_playback_speed"), &TimelineClip::get_playback_speed);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "playback_speed"), "set_playback_speed", "get_playback_speed");

    ClassDB::bind_method(D_METHOD("get_duration"), &TimelineClip::get_duration);
    ClassDB::bind_method(D_METHOD("get_timeline_end"), &TimelineClip::get_timeline_end);

    // Effects bindings
    ClassDB::bind_method(D_METHOD("add_effect", "effect"), &TimelineClip::add_effect);
    ClassDB::bind_method(D_METHOD("remove_effect", "index"), &TimelineClip::remove_effect);
    ClassDB::bind_method(D_METHOD("clear_effects"), &TimelineClip::clear_effects);
    ClassDB::bind_method(D_METHOD("get_effects"), &TimelineClip::get_effects);
    ClassDB::bind_method(D_METHOD("get_effect_count"), &TimelineClip::get_effect_count);
}

TimelineClip::TimelineClip() {}
TimelineClip::~TimelineClip() {}

void TimelineClip::set_source_path(const String &p_path) {
    source_path = p_path;
}

String TimelineClip::get_source_path() const {
    return source_path;
}

void TimelineClip::set_timeline_start(double p_time) {
    timeline_start = p_time;
}

double TimelineClip::get_timeline_start() const {
    return timeline_start;
}

void TimelineClip::set_source_in_point(double p_time) {
    source_in_point = p_time;
}

double TimelineClip::get_source_in_point() const {
    return source_in_point;
}

void TimelineClip::set_source_out_point(double p_time) {
    source_out_point = p_time;
}

double TimelineClip::get_source_out_point() const {
    return source_out_point;
}

void TimelineClip::set_playback_speed(double p_speed) {
    playback_speed = p_speed;
}

double TimelineClip::get_playback_speed() const {
    return playback_speed;
}

double TimelineClip::get_duration() const {
    double source_duration = source_out_point - source_in_point;
    if (source_duration <= 0.0) {
        return 0.0;
    }
    return source_duration / playback_speed;
}

double TimelineClip::get_timeline_end() const {
    return timeline_start + get_duration();
}

// Effects implementations
void TimelineClip::add_effect(const Ref<VideoEffect> &p_effect) {
    if (p_effect.is_valid()) {
        effects.push_back(p_effect);
    }
}

void TimelineClip::remove_effect(int p_index) {
    if (p_index >= 0 && p_index < effects.size()) {
        effects.remove_at(p_index);
    }
}

void TimelineClip::clear_effects() {
    effects.clear();
}

TypedArray<VideoEffect> TimelineClip::get_effects() const {
    return effects;
}

int TimelineClip::get_effect_count() const {
    return effects.size();
}
