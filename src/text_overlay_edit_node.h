#ifndef TEXT_OVERLAY_EDIT_NODE_H
#define TEXT_OVERLAY_EDIT_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include "text_overlay.h"

namespace godot {

class TextOverlayEditNode : public Control {
    GDCLASS(TextOverlayEditNode, Control)

public:
    enum DragMode {
        DRAG_NONE,
        DRAG_MOVE,
        DRAG_SCALE,
        DRAG_ROTATE
    };

private:
    Ref<TextOverlay> text_overlay;
    bool selected = false;

    // Visual settings
    Color selection_border_color = Color(1.0f, 0.85f, 0.2f, 0.9f);
    float selection_border_width = 2.0f;
    Color handle_color = Color(1.0f, 1.0f, 1.0f, 0.9f);
    float handle_radius = 8.0f;

    // Drag state
    DragMode drag_mode = DRAG_NONE;
    Vector2 drag_start_pos;           // local to this node
    Vector2 drag_start_overlay_pos;
    float drag_start_scale = 1.0f;
    float drag_start_rotation = 0.0f;
    float drag_start_angle = 0.0f;    // angle from center to mouse at drag start
    Vector2 drag_start_center;

    // Cache
    Ref<ImageTexture> cached_texture;
    bool texture_dirty = true;

    void _update_texture();
    void _update_size_from_overlay();
    Rect2 _get_content_rect() const;
    Vector2 _get_scale_handle_pos() const;
    Vector2 _get_rotate_handle_pos() const;
    bool _is_near_scale_handle(const Vector2 &p_pos) const;
    bool _is_near_rotate_handle(const Vector2 &p_pos) const;
    void _handle_drag(const Vector2 &p_pos);

protected:
    static void _bind_methods();
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

public:
    void set_text_overlay(const Ref<TextOverlay> &p_overlay);
    Ref<TextOverlay> get_text_overlay() const;

    void set_selected(bool p_selected);
    bool is_selected() const;

    void set_selection_border_color(const Color &p_color);
    Color get_selection_border_color() const;

    void set_handle_color(const Color &p_color);
    Color get_handle_color() const;

    void mark_dirty(); // call when text_overlay properties change externally

    TextOverlayEditNode();
    ~TextOverlayEditNode();
};

}

VARIANT_ENUM_CAST(TextOverlayEditNode::DragMode);

#endif
