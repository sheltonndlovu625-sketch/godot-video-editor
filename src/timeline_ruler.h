#ifndef TIMELINE_RULER_H
#define TIMELINE_RULER_H

#include <godot_cpp/classes/control.hpp>

namespace godot {

class TimelineRuler : public Control {
    GDCLASS(TimelineRuler, Control)

private:
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    double duration = 60.0;
    float header_width = 50.0f;

protected:
    static void _bind_methods();

public:
    void _draw() override;  // <-- MOVED TO PUBLIC

    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_duration(double p_duration);
    double get_duration() const;
    void set_header_width(float p_width);
    float get_header_width() const;

    TimelineRuler();
};

}
#endif
