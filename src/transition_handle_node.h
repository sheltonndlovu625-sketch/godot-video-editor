#ifndef TRANSITION_HANDLE_NODE_H
#define TRANSITION_HANDLE_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>

namespace godot {

class TransitionHandleNode : public Control {
    GDCLASS(TransitionHandleNode, Control)

private:
    bool dragging;
    float drag_start_x;
    float transition_duration;
    float max_duration;
    Color handle_color;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    TransitionHandleNode();
    ~TransitionHandleNode();

    virtual void _gui_input(const Ref<InputEvent> &p_event) override;

    void set_transition_duration(float p_duration);
    float get_transition_duration() const;

    void set_max_duration(float p_max);
    float get_max_duration() const;

    void set_handle_color(const Color &p_color);
    Color get_handle_color() const;
};

}

#endif