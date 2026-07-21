// timeline_zoom_controller.h
#ifndef TIMELINE_ZOOM_CONTROLLER_H
#define TIMELINE_ZOOM_CONTROLLER_H

#include <godot_cpp/classes/node.hpp>

namespace godot {

class TimelineZoomController : public Node {
    GDCLASS(TimelineZoomController, Node)

private:
    float zoom = 1.0f;
    float min_zoom = 0.1f;
    float max_zoom = 10.0f;
    float pixels_per_second = 60.0f;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_min_zoom(float p_min);
    float get_min_zoom() const;
    void set_max_zoom(float p_max);
    float get_max_zoom() const;
    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void zoom_in();
    void zoom_out();
    void apply_zoom_to_children();

    TimelineZoomController();
};

}

#endif
