#ifndef TIMELINE_TRACK_H
#define TIMELINE_TRACK_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "timeline_clip.h"

namespace godot {

enum TimelineTrackType {
    TRACK_TYPE_VIDEO = 0,
    TRACK_TYPE_AUDIO = 1
};

class TimelineTrack : public RefCounted {
    GDCLASS(TimelineTrack, RefCounted)

private:
    TimelineTrackType track_type = TRACK_TYPE_VIDEO;
    int layer_index = 0;  // Higher = on top (for video compositing)
    Vector<Ref<TimelineClip>> clips;

protected:
    static void _bind_methods();

public:
    void set_track_type(int p_type);
    int get_track_type() const;

    void set_layer_index(int p_index);
    int get_layer_index() const;

    void add_clip(const Ref<TimelineClip> &p_clip);
    void remove_clip(int p_index);
    void clear_clips();
    int get_clip_count() const;
    Ref<TimelineClip> get_clip(int p_index) const;

    // Find clip active at given timeline time
    Ref<TimelineClip> get_clip_at_time(double p_time) const;

    // Get all clips sorted by timeline_start
    TypedArray<TimelineClip> get_clips() const;

    double get_track_duration() const; // End time of last clip

    TimelineTrack();
    ~TimelineTrack();
};

}

#endif

