#ifndef TRANSITION_HANDLE_NODE_H
#define TRANSITION_HANDLE_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>
#include "transition_effect.h"

namespace godot {

class TransitionHandleNode : public Control {
    GDCLASS(TransitionHandleNode, Control)

private:
    float transition_duration = 0.0f;
    float max_duration = 5.0f;
    Color handle_color = Color(0.0, 1.0, 0.0, 0.8);

    // Thumbnail / cover image
    String texture_path;
    Ref<Image> cached_image;
    Ref<ImageTexture> cached_texture;
    bool image_loaded = false;

    // Zoom visibility
    float zoom = 1.0f;
    float zoom_threshold = 5.0f;

    // Selection
    bool selected = false;

    // Linked effect (editable in Inspector)
    Ref<TransitionEffect> transition_effect;

    void _handle_press(const Vector2 &p_pos);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    TransitionHandleNode();
    ~TransitionHandleNode();

    virtual void _gui_input(const Ref<InputEvent> &p_event) override;

    // Duration
    void set_transition_duration(float p_duration);
    float get_transition_duration() const;

    void set_max_duration(float p_max);
    float get_max_duration() const;

    // Visuals
    void set_handle_color(const Color &p_color);
    Color get_handle_color() const;

    void set_texture_path(const String &p_path);
    String get_texture_path() const;

    // Zoom visibility
    void set_zoom(float p_zoom);
    float get_zoom() const;

    void set_zoom_threshold(float p_threshold);
    float get_zoom_threshold() const;

    // Selection
    void set_selected(bool p_selected);
    bool is_selected() const;

    // Effect resource
    void set_transition_effect(const Ref<TransitionEffect> &p_effect);
    Ref<TransitionEffect> get_transition_effect() const;
};

}

#endif
