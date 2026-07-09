#ifndef TIMELINE_CLIP_H
#define TIMELINE_CLIP_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class TimelineClip : public RefCounted {
    GDCLASS(TimelineClip, RefCounted)

private:
    String source_path;
    String proxy_path;              // <-- ADDED: 480p preview file
    double timeline_start = 0.0;
    double source_in_point = 0.0;
    double source_out_point = 0.0;
    double playback_speed = 1.0;

protected:
    static void _bind_methods();

public:
    void set_source_path(const String &p_path);
    String get_source_path() const;

    void set_proxy_path(const String &p_path);   // <-- ADDED
    String get_proxy_path() const;               // <-- ADDED
    String get_effective_path(bool p_use_proxy) const; // <-- ADDED

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

    TimelineClip();
    ~TimelineClip();
};

}

#endif
