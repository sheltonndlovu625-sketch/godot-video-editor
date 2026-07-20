#ifndef TIMELINE_CLIP_H
#define TIMELINE_CLIP_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include "video_effect.h"

namespace godot {

class TimelineClip : public RefCounted {
    GDCLASS(TimelineClip, RefCounted)

private:
    String source_path;
    double timeline_start = 0.0;
    double source_in_point = 0.0;
    double source_out_point = 0.0;
    double playback_speed = 1.0;
    TypedArray<VideoEffect> effects;

    // Transform
    Vector2 position;
    Vector2 scale = Vector2(1.0, 1.0);
    float rotation = 0.0f;
    float opacity = 1.0f;
    Vector2 anchor_point = Vector2(0.5, 0.5);

protected:
    static void _bind_methods();

public:
    void set_source_path(const String &p_path);
    String get_source_path() const;

    void set_timeline_start(double p_time);
    double get_timeline_start() const;

    void set_source_in_point(double p_time);
    double get_source_in_point() const;

    void set_source_out_point(double p_time);
    double get_source_out_point() const;

    void set_playback_speed(double p_speed);
    double get_playback_speed() const;

    double get_duration() const;
    double get_timeline_end() const;

    // Effects
    void add_effect(const Ref<VideoEffect> &p_effect);
    void remove_effect(int p_index);
    void clear_effects();
    TypedArray<VideoEffect> get_effects() const;
    int get_effect_count() const;

    // Split
    TypedArray<TimelineClip> split(double p_local_time);

    // Transform
    void set_position(const Vector2 &p_pos);
    Vector2 get_position() const;

    void set_scale(const Vector2 &p_scale);
    Vector2 get_scale() const;

    void set_rotation(float p_rot);
    float get_rotation() const;

    void set_opacity(float p_opacity);
    float get_opacity() const;

    void set_anchor_point(const Vector2 &p_anchor);
    Vector2 get_anchor_point() const;

    TimelineClip();
    ~TimelineClip();
};

}

#endif
