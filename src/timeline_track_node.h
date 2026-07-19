#ifndef TIMELINE_TRACK_NODE_H
#define TIMELINE_TRACK_NODE_H

#include <godot_cpp/classes/control.hpp>
#include "timeline_track.h"

namespace godot {

class TimelineTrackNode : public Control {
    GDCLASS(TimelineTrackNode, Control)

private:
    Ref<TimelineTrack> track;
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    float header_width = 50.0f;
    bool is_video_track = true;

protected:
    static void _bind_methods();
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

public:
    void set_track(const Ref<TimelineTrack> &p_track);
    Ref<TimelineTrack> get_track() const;
    
    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    
    void set_zoom(float p_zoom);
    float get_zoom() const;
    
    void set_header_width(float p_width);
    float get_header_width() const;
    
    TimelineTrackNode();
};

}
#endif
