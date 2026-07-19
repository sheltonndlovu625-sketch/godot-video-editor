#ifndef TIMELINE_CLIP_NODE_H
#define TIMELINE_CLIP_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>
#include "timeline_clip.h"

namespace godot {

class TimelineClipNode : public Control {
    GDCLASS(TimelineClipNode, Control)

public:
    enum DragMode {
        DRAG_NONE,
        DRAG_MOVE,
        DRAG_TRIM_LEFT,
        DRAG_TRIM_RIGHT
    };

private:
    Ref<TimelineClip> clip;
    Color base_color = Color(0.2f, 0.5f, 0.95f);
    bool selected = false;
    DragMode drag_mode = DRAG_NONE;
    Vector2 drag_start_pos;
    double drag_start_timeline_start = 0.0;
    double drag_start_duration = 0.0;
    double drag_start_source_in = 0.0;
    double drag_start_source_out = 0.0;
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    float handle_width = 12.0f;
    bool is_video = true;

protected:
    static void _bind_methods();

public:
    void _draw() override;                              // <-- PUBLIC
    void _gui_input(const Ref<InputEvent> &p_event) override;  // <-- PUBLIC

    void set_clip(const Ref<TimelineClip> &p_clip);
    Ref<TimelineClip> get_clip() const;
    void set_selected(bool p_selected);
    bool is_selected() const;
    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_is_video(bool p_video);
    bool get_is_video() const;
    void update_layout();

    TimelineClipNode();
};

}

VARIANT_ENUM_CAST(TimelineClipNode::DragMode);

#endif
