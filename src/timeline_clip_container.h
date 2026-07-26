#ifndef TIMELINE_CLIP_CONTAINER_H
#define TIMELINE_CLIP_CONTAINER_H

#include <godot_cpp/classes/control.hpp>
#include "timeline_track.h"
#include "timeline_clip_node.h"

namespace godot {

class TimelineClipContainer : public Control {
    GDCLASS(TimelineClipContainer, Control)

private:
    Ref<TimelineTrack> track;
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    float header_width = 50.0f;

    void _clear_clip_nodes();
    void _create_clip_nodes();
    void _update_layout();

protected:
    static void _bind_methods();

public:
    void set_track(const Ref<TimelineTrack> &p_track);
    Ref<TimelineTrack> get_track() const;

    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;

    void set_zoom(float p_zoom);
    float get_zoom() const;

    void set_header_width(float p_width);
    float get_header_width() const;

    // Call after external clip changes (drag finished, undo, etc.)
    void refresh();

    TimelineClipContainer();
};

}

#endif
