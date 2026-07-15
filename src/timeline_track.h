#ifndef TIMELINE_TRACK_H
#define TIMELINE_TRACK_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "timeline_clip.h"

namespace godot {

class TimelineTrack : public RefCounted {
    GDCLASS(TimelineTrack, RefCounted)

public:
    enum TrackType {
        TRACK_TYPE_VIDEO = 0,
        TRACK_TYPE_AUDIO = 1
    };

    enum BlendMode {
        BLEND_MODE_NORMAL = 0,
        BLEND_MODE_ADD = 1,
        BLEND_MODE_MULTIPLY = 2,
        BLEND_MODE_SUBTRACT = 3
    };

private:
    TrackType track_type = TRACK_TYPE_VIDEO;
    int layer_index = 0;
    Vector<Ref<TimelineClip>> clips;
    BlendMode blend_mode = BLEND_MODE_NORMAL;

protected:
    static void _bind_methods();

public:
    void set_track_type(int p_type);
    int get_track_type() const;

    void set_layer_index(int p_index);
    int get_layer_index() const;

    void set_blend_mode(int p_mode);
    int get_blend_mode() const;

    void add_clip(const Ref<TimelineClip> &p_clip);
    void remove_clip(int p_index);
    void clear_clips();
    int get_clip_count() const;
    Ref<TimelineClip> get_clip(int p_index) const;
    Ref<TimelineClip> get_clip_at_time(double p_time) const;
    TypedArray<TimelineClip> get_clips() const;
    double get_track_duration() const;

    TimelineTrack();
    ~TimelineTrack();
};

}

VARIANT_ENUM_CAST(godot::TimelineTrack::TrackType);
VARIANT_ENUM_CAST(godot::TimelineTrack::BlendMode);

#endif
