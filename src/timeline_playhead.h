// timeline_playhead.h
#ifndef TIMELINE_PLAYHEAD_H
#define TIMELINE_PLAYHEAD_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class TimelinePlayhead : public Control {
    GDCLASS(TimelinePlayhead, Control)

private:
    float header_width = 50.0f;
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    double current_time = 0.0;
    double duration = 60.0;
    bool dragging = false;
    Color line_color = Color(1.0f, 0.2f, 0.2f, 0.9f);

protected:
    static void _bind_methods();

public:
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

    void set_time(double p_time);
    double get_time() const;
    void set_duration(double p_duration);
    double get_duration() const;
    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_header_width(float p_width);
    float get_header_width() const;

    TimelinePlayhead();
};

}

#endif
