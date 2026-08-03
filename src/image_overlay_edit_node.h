#ifndef IMAGE_OVERLAY_EDIT_NODE_H
#define IMAGE_OVERLAY_EDIT_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include "image_overlay.h"

namespace godot {

class ImageOverlayEditNode : public Control {
    GDCLASS(ImageOverlayEditNode, Control)

public:
    enum DragMode {
        DRAG_NONE,
        DRAG_MOVE,
        DRAG_SCALE,
        DRAG_ROTATE
    };

private:
    Ref<ImageOverlay> image_overlay;
    bool selected = false;

    // Visual settings
    Color selection_border_color = Color(1.0f, 0.85f, 0.2f, 0.9f);
    float selection_border_width = 2.0f;
    Color handle_color = Color(1.0f, 1.0f, 1.0f, 0.9f);
    float handle_radius = 8.0f;

    // Drag state (all parent-space math)
    DragMode drag_mode = DRAG_NONE;
    Vector2 drag_start_local_pos;
    Vector2 drag_start_parent_mouse_pos;
    Vector2 drag_start_overlay_pos;
    float drag_start_scale = 1.0f;
    float drag_start_rotation = 0.0f;
    float drag_start_angle = 0.0f;
    Vector2 drag_start_center;

    // Cache
    Ref<ImageTexture> cached_texture;
    Vector2 cached_image_size;
    bool texture_dirty = true;

    // Auto-sync tracking
    Vector2 last_overlay_pos;
    Vector2 last_overlay_scale;
    float last_overlay_rotation = 0.0f;
    Vector2 last_overlay_anchor;

    void _update_texture();
    void _update_size_from_overlay();
    Rect2 _get_content_rect() const;
    Vector2 _get_scale_handle_pos() const;
    Vector2 _get_rotate_handle_pos() const;
    bool _is_near_scale_handle(const Vector2 &p_local_pos) const;
    bool _is_near_rotate_handle(const Vector2 &p_local_pos) const;
    void _handle_drag(const Vector2 &p_local_pos);
    void _draw_dashed_rect(const Rect2 &p_rect, const Color &p_color, float p_width, float p_dash, float p_gap);

protected:
    static void _bind_methods();

public:
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;
    void _process(double delta) override;

    void set_image_overlay(const Ref<ImageOverlay> &p_overlay);
    Ref<ImageOverlay> get_image_overlay() const;

    void set_selected(bool p_selected);
    bool is_selected() const;

    void set_selection_border_color(const Color &p_color);
    Color get_selection_border_color() const;
    void set_handle_color(const Color &p_color);
    Color get_handle_color() const;
    void set_handle_radius(float p_radius);
    float get_handle_radius() const;

    void mark_dirty();

    ImageOverlayEditNode();
    ~ImageOverlayEditNode();
};

}

VARIANT_ENUM_CAST(ImageOverlayEditNode::DragMode);

#endif
