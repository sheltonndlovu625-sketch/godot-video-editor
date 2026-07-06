#ifndef TIMELINE_CLIP_H
#define TIMELINE_CLIP_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class TimelineClip : public RefCounted {
    GDCLASS(TimelineClip, RefCounted)

private:
    String source_path;
    double timeline_start = 0.0;   // When this clip starts on the timeline (seconds)
    double source_in_point = 0.0;  // Trim: start reading from source at this time
    double source_out_point = 0.0; // Trim: stop reading at this time (0 = use full duration)
    double playback_speed = 1.0;   // 1.0 = normal, 2.0 = double speed, 0.5 = half speed

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

    double get_duration() const;   // source_out_point - source_in_point (adjusted for speed)
    double get_timeline_end() const; // timeline_start + duration

    TimelineClip();
    ~TimelineClip();
};

}

#endif

