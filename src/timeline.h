#ifndef TIMELINE_H
#define TIMELINE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "timeline_track.h"

namespace godot {

class Timeline : public RefCounted {
    GDCLASS(Timeline, RefCounted)

private:
    Vector<Ref<TimelineTrack>> tracks;
    double playhead_time = 0.0;
    double frame_rate = 30.0;

protected:
    static void _bind_methods();

public:
    void add_track(const Ref<TimelineTrack> &p_track);
    void remove_track(int p_index);
    void clear_tracks();
    int get_track_count() const;
    Ref<TimelineTrack> get_track(int p_index) const;

    // Get tracks by type
    TypedArray<TimelineTrack> get_video_tracks() const;
    TypedArray<TimelineTrack> get_audio_tracks() const;

    void set_playhead_time(double p_time);
    double get_playhead_time() const;

    void set_frame_rate(double p_fps);
    double get_frame_rate() const;

    double get_duration() const; // Max duration across all tracks

    // Frame-based playhead
    void set_playhead_frame(int p_frame);
    int get_playhead_frame() const;

    // Step forward/back one frame
    void step_forward();
    void step_backward();

    // Check if playhead is at end
    bool is_at_end() const;

    Timeline();
    ~Timeline();
};

}

#endif

