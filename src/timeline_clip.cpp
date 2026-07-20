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

    // Split binding
    ClassDB::bind_method(D_METHOD("split", "local_time"), &TimelineClip::split);

    // Transform bindings
    ClassDB::bind_method(D_METHOD("set_position", "pos"), &TimelineClip::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &TimelineClip::get_position);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::VECTOR2, "position"), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("set_scale", "scale"), &TimelineClip::set_scale);
    ClassDB::bind_method(D_METHOD("get_scale"), &TimelineClip::get_scale);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::VECTOR2, "scale"), "set_scale", "get_scale");

    ClassDB::bind_method(D_METHOD("set_rotation", "rotation"), &TimelineClip::set_rotation);
    ClassDB::bind_method(D_METHOD("get_rotation"), &TimelineClip::get_rotation);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "rotation"), "set_rotation", "get_rotation");

    ClassDB::bind_method(D_METHOD("set_opacity", "opacity"), &TimelineClip::set_opacity);
    ClassDB::bind_method(D_METHOD("get_opacity"), &TimelineClip::get_opacity);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::FLOAT, "opacity", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_opacity", "get_opacity");

    ClassDB::bind_method(D_METHOD("set_anchor_point", "anchor"), &TimelineClip::set_anchor_point);
    ClassDB::bind_method(D_METHOD("get_anchor_point"), &TimelineClip::get_anchor_point);
    ClassDB::add_property("TimelineClip", PropertyInfo(Variant::VECTOR2, "anchor_point"), "set_anchor_point", "get_anchor_point");
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

// Split implementation
TypedArray<TimelineClip> TimelineClip::split(double p_local_time) {
    TypedArray<TimelineClip> result;
    double dur = get_duration();
    if (p_local_time <= 0.0 || p_local_time >= dur || dur <= 0.0) {
        return result;
    }

    double source_split = source_in_point + p_local_time * playback_speed;

    Ref<TimelineClip> first;
    first.instantiate();
    first->set_source_path(source_path);
    first->set_timeline_start(timeline_start);
    first->set_source_in_point(source_in_point);
    first->set_source_out_point(source_split);
    first->set_playback_speed(playback_speed);
    first->set_position(position);
    first->set_scale(scale);
    first->set_rotation(rotation);
    first->set_opacity(opacity);
    first->set_anchor_point(anchor_point);

    TypedArray<VideoEffect> fx = get_effects();
    for (int i = 0; i < fx.size(); i++) {
        first->add_effect(fx[i]);
    }

    Ref<TimelineClip> second;
    second.instantiate();
    second->set_source_path(source_path);
    second->set_timeline_start(timeline_start + p_local_time);
    second->set_source_in_point(source_split);
    second->set_source_out_point(source_out_point);
    second->set_playback_speed(playback_speed);
    second->set_position(position);
    second->set_scale(scale);
    second->set_rotation(rotation);
    second->set_opacity(opacity);
    second->set_anchor_point(anchor_point);

    for (int i = 0; i < fx.size(); i++) {
        second->add_effect(fx[i]);
    }

    result.push_back(first);
    result.push_back(second);
    return result;
}

// Transform implementations
void TimelineClip::set_position(const Vector2 &p_pos) {
    position = p_pos;
}

Vector2 TimelineClip::get_position() const {
    return position;
}

void TimelineClip::set_scale(const Vector2 &p_scale) {
    scale = p_scale;
}

Vector2 TimelineClip::get_scale() const {
    return scale;
}

void TimelineClip::set_rotation(float p_rot) {
    rotation = p_rot;
}

float TimelineClip::get_rotation() const {
    return rotation;
}

void TimelineClip::set_opacity(float p_opacity) {
    opacity = CLAMP(p_opacity, 0.0f, 1.0f);
}

float TimelineClip::get_opacity() const {
    return opacity;
}

void TimelineClip::set_anchor_point(const Vector2 &p_anchor) {
    anchor_point = p_anchor;
}

Vector2 TimelineClip::get_anchor_point() const {
    return anchor_point;
}
