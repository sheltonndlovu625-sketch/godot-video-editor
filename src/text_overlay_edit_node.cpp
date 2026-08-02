#include "text_overlay_edit_node.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void TextOverlayEditNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_text_overlay", "overlay"), &TextOverlayEditNode::set_text_overlay);
    ClassDB::bind_method(D_METHOD("get_text_overlay"), &TextOverlayEditNode::get_text_overlay);
    ClassDB::add_property("TextOverlayEditNode", PropertyInfo(Variant::OBJECT, "text_overlay", PROPERTY_HINT_RESOURCE_TYPE, "TextOverlay"), "set_text_overlay", "get_text_overlay");

    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &TextOverlayEditNode::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &TextOverlayEditNode::is_selected);
    ClassDB::add_property("TextOverlayEditNode", PropertyInfo(Variant::BOOL, "selected"), "set_selected", "is_selected");

    ClassDB::bind_method(D_METHOD("set_selection_border_color", "color"), &TextOverlayEditNode::set_selection_border_color);
    ClassDB::bind_method(D_METHOD("get_selection_border_color"), &TextOverlayEditNode::get_selection_border_color);
    ClassDB::add_property("TextOverlayEditNode", PropertyInfo(Variant::COLOR, "selection_border_color"), "set_selection_border_color", "get_selection_border_color");

    ClassDB::bind_method(D_METHOD("set_handle_color", "color"), &TextOverlayEditNode::set_handle_color);
    ClassDB::bind_method(D_METHOD("get_handle_color"), &TextOverlayEditNode::get_handle_color);
    ClassDB::add_property("TextOverlayEditNode", PropertyInfo(Variant::COLOR, "handle_color"), "set_handle_color", "get_handle_color");

    ClassDB::bind_method(D_METHOD("mark_dirty"), &TextOverlayEditNode::mark_dirty);

    ADD_SIGNAL(MethodInfo("selected"));
    ADD_SIGNAL(MethodInfo("deselected"));
    ADD_SIGNAL(MethodInfo("position_changed", PropertyInfo(Variant::VECTOR2, "new_position")));
    ADD_SIGNAL(MethodInfo("scale_changed", PropertyInfo(Variant::FLOAT, "new_scale")));
    ADD_SIGNAL(MethodInfo("rotation_changed", PropertyInfo(Variant::FLOAT, "new_rotation")));

    BIND_ENUM_CONSTANT(DRAG_NONE);
    BIND_ENUM_CONSTANT(DRAG_MOVE);
    BIND_ENUM_CONSTANT(DRAG_SCALE);
    BIND_ENUM_CONSTANT(DRAG_ROTATE);
}

TextOverlayEditNode::TextOverlayEditNode() {
    set_mouse_filter(MOUSE_FILTER_STOP);
}

TextOverlayEditNode::~TextOverlayEditNode() {}

void TextOverlayEditNode::set_text_overlay(const Ref<TextOverlay> &p_overlay) {
    if (text_overlay == p_overlay) return;
    text_overlay = p_overlay;
    texture_dirty = true;
    _update_size_from_overlay();
    queue_redraw();
}

Ref<TextOverlay> TextOverlayEditNode::get_text_overlay() const {
    return text_overlay;
}

void TextOverlayEditNode::set_selected(bool p_selected) {
    if (selected == p_selected) return;
    selected = p_selected;
    queue_redraw();
    if (selected) emit_signal("selected");
    else emit_signal("deselected");
}

bool TextOverlayEditNode::is_selected() const {
    return selected;
}

void TextOverlayEditNode::set_selection_border_color(const Color &p_color) {
    selection_border_color = p_color;
    if (selected) queue_redraw();
}

Color TextOverlayEditNode::get_selection_border_color() const {
    return selection_border_color;
}

void TextOverlayEditNode::set_handle_color(const Color &p_color) {
    handle_color = p_color;
    if (selected) queue_redraw();
}

Color TextOverlayEditNode::get_handle_color() const {
    return handle_color;
}

void TextOverlayEditNode::mark_dirty() {
    texture_dirty = true;
    _update_size_from_overlay();
    queue_redraw();
}

void TextOverlayEditNode::_update_texture() {
    if (!text_overlay.is_valid()) return;
    Ref<Image> img = text_overlay->render_to_image();

    // FIX: render_to_image returns null when the viewport hasn't rendered yet
    // (VIEWPORT_UPDATE_ONCE draws at end of frame). Keep dirty and retry.
    if (img.is_null() || img->get_width() == 0 || img->get_height() == 0) {
        texture_dirty = true;
        return;
    }

    if (cached_texture.is_null()) {
        cached_texture.instantiate();
    }
    cached_texture->set_image(img);
    texture_dirty = false;

    // FIX: size may have been 0x0 before the texture existed; recalculate now.
    _update_size_from_overlay();
}

void TextOverlayEditNode::_update_size_from_overlay() {
    if (!text_overlay.is_valid()) return;
    Vector2 render_size = text_overlay->get_render_size();
    float s = text_overlay->get_scale();
    set_size(render_size * s);
    set_position(text_overlay->get_position() - get_size() * text_overlay->get_anchor_point());
}

Rect2 TextOverlayEditNode::_get_content_rect() const {
    return Rect2(Vector2(), get_size());
}

Vector2 TextOverlayEditNode::_get_scale_handle_pos() const {
    Rect2 r = _get_content_rect();
    return r.position + r.size; // bottom-right
}

Vector2 TextOverlayEditNode::_get_rotate_handle_pos() const {
    Rect2 r = _get_content_rect();
    return Vector2(r.position.x + r.size.x * 0.5f, r.position.y - handle_radius * 2.5f);
}

bool TextOverlayEditNode::_is_near_scale_handle(const Vector2 &p_pos) const {
    return p_pos.distance_to(_get_scale_handle_pos()) <= handle_radius * 1.8f;
}

bool TextOverlayEditNode::_is_near_rotate_handle(const Vector2 &p_pos) const {
    return p_pos.distance_to(_get_rotate_handle_pos()) <= handle_radius * 1.8f;
}

void TextOverlayEditNode::_draw() {
    if (texture_dirty) _update_texture();
    if (!text_overlay.is_valid()) return;

    // Draw the text texture (rotated around center if needed)
    if (cached_texture.is_valid()) {
        float rot = text_overlay->get_rotation();
        if (Math::abs(rot) > 0.001f) {
            Vector2 center = get_size() * 0.5f;
            draw_set_transform(center, rot, Vector2(1, 1));
            draw_texture_rect(cached_texture, Rect2(-center, get_size()), false);
            draw_set_transform(Vector2(), 0, Vector2(1, 1));
        } else {
            draw_texture_rect(cached_texture, _get_content_rect(), false);
        }
    }

    if (!selected) return;

    Rect2 content = _get_content_rect();

    // Selection border
    draw_rect(content, selection_border_color, false, selection_border_width);

    // Scale handle (bottom-right)
    Vector2 scale_pos = _get_scale_handle_pos();
    draw_circle(scale_pos, handle_radius, handle_color);
    draw_line(scale_pos - Vector2(4, 0), scale_pos + Vector2(4, 0), Color(0, 0, 0, 0.8f), 1.5f);
    draw_line(scale_pos - Vector2(0, 4), scale_pos + Vector2(0, 4), Color(0, 0, 0, 0.8f), 1.5f);

    // Rotate handle (top-center)
    Vector2 rotate_pos = _get_rotate_handle_pos();
    draw_circle(rotate_pos, handle_radius, handle_color);
    draw_arc(rotate_pos, 4.0f, 0.0f, Math_PI * 1.5f, 8, Color(0, 0, 0, 0.8f), 1.5f);
    // Line connecting rotate handle to content
    draw_line(Vector2(content.position.x + content.size.x * 0.5f, content.position.y), rotate_pos, handle_color, 1.0f);
}

void TextOverlayEditNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (!text_overlay.is_valid()) return;

    // ---- Touch support ----
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            Vector2 pos = touch->get_position();
            if (_is_near_scale_handle(pos)) {
                drag_mode = DRAG_SCALE;
                drag_start_pos = pos;
                drag_start_scale = text_overlay->get_scale();
                drag_start_center = get_size() * 0.5f;
            } else if (_is_near_rotate_handle(pos)) {
                drag_mode = DRAG_ROTATE;
                drag_start_pos = pos;
                drag_start_rotation = text_overlay->get_rotation();
                Vector2 center = get_size() * 0.5f;
                drag_start_angle = (pos - center).angle();
            } else if (_get_content_rect().has_point(pos)) {
                drag_mode = DRAG_MOVE;
                drag_start_pos = pos;
                drag_start_overlay_pos = text_overlay->get_position();
                if (!selected) set_selected(true);
            } else {
                if (selected) set_selected(false);
            }
        } else {
            drag_mode = DRAG_NONE;
        }
        return;
    }

    Ref<InputEventScreenDrag> touch_drag = p_event;
    if (touch_drag.is_valid() && touch_drag->get_index() == 0 && drag_mode != DRAG_NONE) {
        _handle_drag(touch_drag->get_position());
        return;
    }

    // ---- Mouse support ----
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (mb->is_pressed()) {
            Vector2 pos = mb->get_position();
            if (_is_near_scale_handle(pos)) {
                drag_mode = DRAG_SCALE;
                drag_start_pos = pos;
                drag_start_scale = text_overlay->get_scale();
                drag_start_center = get_size() * 0.5f;
            } else if (_is_near_rotate_handle(pos)) {
                drag_mode = DRAG_ROTATE;
                drag_start_pos = pos;
                drag_start_rotation = text_overlay->get_rotation();
                Vector2 center = get_size() * 0.5f;
                drag_start_angle = (pos - center).angle();
            } else if (_get_content_rect().has_point(pos)) {
                drag_mode = DRAG_MOVE;
                drag_start_pos = pos;
                drag_start_overlay_pos = text_overlay->get_position();
                if (!selected) set_selected(true);
            } else {
                if (selected) set_selected(false);
            }
        } else {
            drag_mode = DRAG_NONE;
        }
        return;
    }

    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && drag_mode != DRAG_NONE) {
        _handle_drag(mm->get_position());
    }
}

void TextOverlayEditNode::_handle_drag(const Vector2 &p_pos) {
    switch (drag_mode) {
        case DRAG_MOVE: {
            Vector2 delta = p_pos - drag_start_pos;
            Vector2 new_pos = drag_start_overlay_pos + delta;
            text_overlay->set_position(new_pos);
            _update_size_from_overlay();
            emit_signal("position_changed", new_pos);
            queue_redraw();
            break;
        }
        case DRAG_SCALE: {
            float start_dist = drag_start_pos.distance_to(drag_start_center);
            float curr_dist = p_pos.distance_to(drag_start_center);
            if (start_dist > 4.0f) {
                float new_scale = drag_start_scale * (curr_dist / start_dist);
                new_scale = CLAMP(new_scale, 0.1f, 10.0f);
                text_overlay->set_scale(new_scale);
                _update_size_from_overlay();
                emit_signal("scale_changed", new_scale);
                queue_redraw();
            }
            break;
        }
        case DRAG_ROTATE: {
            Vector2 center = get_size() * 0.5f;
            float current_angle = (p_pos - center).angle();
            float delta_angle = current_angle - drag_start_angle;
            float new_rotation = drag_start_rotation + delta_angle;
            text_overlay->set_rotation(new_rotation);
            emit_signal("rotation_changed", new_rotation);
            queue_redraw();
            break;
        }
        default:
            break;
    }
}
