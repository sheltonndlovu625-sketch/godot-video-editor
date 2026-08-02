#include "image_overlay_edit_node.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ImageOverlayEditNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_image_overlay", "overlay"), &ImageOverlayEditNode::set_image_overlay);
    ClassDB::bind_method(D_METHOD("get_image_overlay"), &ImageOverlayEditNode::get_image_overlay);
    ClassDB::add_property("ImageOverlayEditNode", PropertyInfo(Variant::OBJECT, "image_overlay", PROPERTY_HINT_RESOURCE_TYPE, "ImageOverlay"), "set_image_overlay", "get_image_overlay");

    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &ImageOverlayEditNode::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &ImageOverlayEditNode::is_selected);
    ClassDB::add_property("ImageOverlayEditNode", PropertyInfo(Variant::BOOL, "selected"), "set_selected", "is_selected");

    ClassDB::bind_method(D_METHOD("set_selection_border_color", "color"), &ImageOverlayEditNode::set_selection_border_color);
    ClassDB::bind_method(D_METHOD("get_selection_border_color"), &ImageOverlayEditNode::get_selection_border_color);
    ClassDB::add_property("ImageOverlayEditNode", PropertyInfo(Variant::COLOR, "selection_border_color"), "set_selection_border_color", "get_selection_border_color");

    ClassDB::bind_method(D_METHOD("set_handle_color", "color"), &ImageOverlayEditNode::set_handle_color);
    ClassDB::bind_method(D_METHOD("get_handle_color"), &ImageOverlayEditNode::get_handle_color);
    ClassDB::add_property("ImageOverlayEditNode", PropertyInfo(Variant::COLOR, "handle_color"), "set_handle_color", "get_handle_color");

    ClassDB::bind_method(D_METHOD("set_handle_radius", "radius"), &ImageOverlayEditNode::set_handle_radius);
    ClassDB::bind_method(D_METHOD("get_handle_radius"), &ImageOverlayEditNode::get_handle_radius);
    ClassDB::add_property("ImageOverlayEditNode", PropertyInfo(Variant::FLOAT, "handle_radius"), "set_handle_radius", "get_handle_radius");

    ClassDB::bind_method(D_METHOD("mark_dirty"), &ImageOverlayEditNode::mark_dirty);

    ADD_SIGNAL(MethodInfo("selected"));
    ADD_SIGNAL(MethodInfo("deselected"));
    ADD_SIGNAL(MethodInfo("position_changed", PropertyInfo(Variant::VECTOR2, "new_position")));
    ADD_SIGNAL(MethodInfo("scale_changed", PropertyInfo(Variant::VECTOR2, "new_scale")));
    ADD_SIGNAL(MethodInfo("rotation_changed", PropertyInfo(Variant::FLOAT, "new_rotation")));

    BIND_ENUM_CONSTANT(DRAG_NONE);
    BIND_ENUM_CONSTANT(DRAG_MOVE);
    BIND_ENUM_CONSTANT(DRAG_SCALE);
    BIND_ENUM_CONSTANT(DRAG_ROTATE);
}

ImageOverlayEditNode::ImageOverlayEditNode() {
    set_mouse_filter(MOUSE_FILTER_STOP);
}

ImageOverlayEditNode::~ImageOverlayEditNode() {}

void ImageOverlayEditNode::set_image_overlay(const Ref<ImageOverlay> &p_overlay) {
    if (image_overlay == p_overlay) return;
    image_overlay = p_overlay;
    texture_dirty = true;
    _update_size_from_overlay();
    queue_redraw();
}

Ref<ImageOverlay> ImageOverlayEditNode::get_image_overlay() const {
    return image_overlay;
}

void ImageOverlayEditNode::set_selected(bool p_selected) {
    if (selected == p_selected) return;
    selected = p_selected;
    queue_redraw();
    if (selected) emit_signal("selected");
    else emit_signal("deselected");
}

bool ImageOverlayEditNode::is_selected() const {
    return selected;
}

void ImageOverlayEditNode::set_selection_border_color(const Color &p_color) {
    selection_border_color = p_color;
    if (selected) queue_redraw();
}

Color ImageOverlayEditNode::get_selection_border_color() const {
    return selection_border_color;
}

void ImageOverlayEditNode::set_handle_color(const Color &p_color) {
    handle_color = p_color;
    if (selected) queue_redraw();
}

Color ImageOverlayEditNode::get_handle_color() const {
    return handle_color;
}

void ImageOverlayEditNode::set_handle_radius(float p_radius) {
    handle_radius = Math::max(2.0f, p_radius);
    if (selected) queue_redraw();
}

float ImageOverlayEditNode::get_handle_radius() const {
    return handle_radius;
}

void ImageOverlayEditNode::mark_dirty() {
    texture_dirty = true;
    _update_size_from_overlay();
    queue_redraw();
}

void ImageOverlayEditNode::_update_texture() {
    if (!image_overlay.is_valid()) return;
    String path = image_overlay->get_texture_path();
    if (path.is_empty()) return;

    String resolved = path;
    if (resolved.begins_with("user://") || resolved.begins_with("res://")) {
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (ps) resolved = ps->globalize_path(resolved);
    }

    Ref<Image> img = Image::load_from_file(resolved);
    if (img.is_null()) return;

    cached_image_size = Vector2((float)img->get_width(), (float)img->get_height());

    if (cached_texture.is_null()) {
        cached_texture.instantiate();
    }
    cached_texture->set_image(img);
    texture_dirty = false;
}

void ImageOverlayEditNode::_update_size_from_overlay() {
    if (!image_overlay.is_valid()) return;
    if (cached_image_size.x <= 0.0f || cached_image_size.y <= 0.0f) {
        _update_texture();
    }
    if (cached_image_size.x <= 0.0f || cached_image_size.y <= 0.0f) return;

    Vector2 scl = image_overlay->get_scale();
    set_size(cached_image_size * scl);

    Vector2 pos = image_overlay->get_position();
    Vector2 anchor = image_overlay->get_anchor_point();
    set_position(pos - get_size() * anchor);
}

Rect2 ImageOverlayEditNode::_get_content_rect() const {
    return Rect2(Vector2(), get_size());
}

Vector2 ImageOverlayEditNode::_get_scale_handle_pos() const {
    Rect2 r = _get_content_rect();
    return r.position + r.size; // bottom-right
}

Vector2 ImageOverlayEditNode::_get_rotate_handle_pos() const {
    Rect2 r = _get_content_rect();
    return Vector2(r.position.x + r.size.x * 0.5f, r.position.y - handle_radius * 2.5f);
}

bool ImageOverlayEditNode::_is_near_scale_handle(const Vector2 &p_pos) const {
    return p_pos.distance_to(_get_scale_handle_pos()) <= handle_radius * 1.8f;
}

bool ImageOverlayEditNode::_is_near_rotate_handle(const Vector2 &p_pos) const {
    return p_pos.distance_to(_get_rotate_handle_pos()) <= handle_radius * 1.8f;
}

void ImageOverlayEditNode::_draw_dashed_rect(const Rect2 &p_rect, const Color &p_color, float p_width, float p_dash, float p_gap) {
    float x = p_rect.position.x;
    float y = p_rect.position.y;
    float w = p_rect.size.x;
    float h = p_rect.size.y;

    float cx = x;
    while (cx < x + w) {
        float seg = Math::min(p_dash, x + w - cx);
        draw_line(Vector2(cx, y), Vector2(cx + seg, y), p_color, p_width);
        cx += p_dash + p_gap;
    }
    cx = x;
    while (cx < x + w) {
        float seg = Math::min(p_dash, x + w - cx);
        draw_line(Vector2(cx, y + h), Vector2(cx + seg, y + h), p_color, p_width);
        cx += p_dash + p_gap;
    }
    float cy = y;
    while (cy < y + h) {
        float seg = Math::min(p_dash, y + h - cy);
        draw_line(Vector2(x, cy), Vector2(x, cy + seg), p_color, p_width);
        cy += p_dash + p_gap;
    }
    cy = y;
    while (cy < y + h) {
        float seg = Math::min(p_dash, y + h - cy);
        draw_line(Vector2(x + w, cy), Vector2(x + w, cy + seg), p_color, p_width);
        cy += p_dash + p_gap;
    }
}

void ImageOverlayEditNode::_draw() {
    if (texture_dirty) _update_texture();
    if (!image_overlay.is_valid()) return;

    if (cached_texture.is_valid()) {
        float rot = image_overlay->get_rotation();
        float op = image_overlay->get_opacity();
        Color modulate = Color(1.0f, 1.0f, 1.0f, op);

        if (Math::abs(rot) > 0.001f) {
            Vector2 center = get_size() * 0.5f;
            draw_set_transform(center, rot, Vector2(1, 1));
            draw_texture_rect(cached_texture, Rect2(-center, get_size()), false, modulate);
            draw_set_transform(Vector2(), 0, Vector2(1, 1));
        } else {
            draw_texture_rect(cached_texture, _get_content_rect(), false, modulate);
        }
    }

    if (!selected) return;

    Rect2 content = _get_content_rect();

    // Dashed reference box
    _draw_dashed_rect(content, selection_border_color, selection_border_width, 6.0f, 4.0f);

    // Corner handles (visual)
    float hr = handle_radius;
    draw_circle(content.position, hr, handle_color); // TL
    draw_circle(content.position + Vector2(content.size.x, 0), hr, handle_color); // TR
    draw_circle(content.position + Vector2(0, content.size.y), hr, handle_color); // BL

    // Interactive scale handle (BR)
    Vector2 scale_pos = _get_scale_handle_pos();
    draw_circle(scale_pos, hr, handle_color);
    draw_line(scale_pos - Vector2(4, 0), scale_pos + Vector2(4, 0), Color(0, 0, 0, 0.8f), 1.5f);
    draw_line(scale_pos - Vector2(0, 4), scale_pos + Vector2(0, 4), Color(0, 0, 0, 0.8f), 1.5f);

    // Interactive rotate handle (top-center)
    Vector2 rotate_pos = _get_rotate_handle_pos();
    draw_circle(rotate_pos, hr, handle_color);
    draw_arc(rotate_pos, 4.0f, 0.0f, Math_PI * 1.5f, 8, Color(0, 0, 0, 0.8f), 1.5f);
    draw_line(Vector2(content.position.x + content.size.x * 0.5f, content.position.y), rotate_pos, handle_color, 1.0f);
}

void ImageOverlayEditNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (!image_overlay.is_valid()) return;

    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            Vector2 pos = touch->get_position();
            if (_is_near_scale_handle(pos)) {
                drag_mode = DRAG_SCALE;
                drag_start_pos = pos;
                drag_start_scale = image_overlay->get_scale().x;
                drag_start_center = get_size() * 0.5f;
            } else if (_is_near_rotate_handle(pos)) {
                drag_mode = DRAG_ROTATE;
                drag_start_pos = pos;
                drag_start_rotation = image_overlay->get_rotation();
                Vector2 center = get_size() * 0.5f;
                drag_start_angle = (pos - center).angle();
            } else if (_get_content_rect().has_point(pos)) {
                drag_mode = DRAG_MOVE;
                drag_start_pos = pos;
                drag_start_overlay_pos = image_overlay->get_position();
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

    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (mb->is_pressed()) {
            Vector2 pos = mb->get_position();
            if (_is_near_scale_handle(pos)) {
                drag_mode = DRAG_SCALE;
                drag_start_pos = pos;
                drag_start_scale = image_overlay->get_scale().x;
                drag_start_center = get_size() * 0.5f;
            } else if (_is_near_rotate_handle(pos)) {
                drag_mode = DRAG_ROTATE;
                drag_start_pos = pos;
                drag_start_rotation = image_overlay->get_rotation();
                Vector2 center = get_size() * 0.5f;
                drag_start_angle = (pos - center).angle();
            } else if (_get_content_rect().has_point(pos)) {
                drag_mode = DRAG_MOVE;
                drag_start_pos = pos;
                drag_start_overlay_pos = image_overlay->get_position();
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

void ImageOverlayEditNode::_handle_drag(const Vector2 &p_pos) {
    switch (drag_mode) {
        case DRAG_MOVE: {
            Vector2 delta = p_pos - drag_start_pos;
            Vector2 new_pos = drag_start_overlay_pos + delta;
            image_overlay->set_position(new_pos);
            _update_size_from_overlay();
            emit_signal("position_changed", new_pos);
            queue_redraw();
            break;
        }
        case DRAG_SCALE: {
            float start_dist = drag_start_pos.distance_to(drag_start_center);
            float curr_dist = p_pos.distance_to(drag_start_center);
            if (start_dist > 4.0f) {
                float ratio = curr_dist / start_dist;
                float new_scale = drag_start_scale * ratio;
                new_scale = CLAMP(new_scale, 0.1f, 10.0f);
                image_overlay->set_scale(Vector2(new_scale, new_scale));
                _update_size_from_overlay();
                emit_signal("scale_changed", Vector2(new_scale, new_scale));
                queue_redraw();
            }
            break;
        }
        case DRAG_ROTATE: {
            Vector2 center = get_size() * 0.5f;
            float current_angle = (p_pos - center).angle();
            float delta_angle = current_angle - drag_start_angle;
            float new_rotation = drag_start_rotation + delta_angle;
            image_overlay->set_rotation(new_rotation);
            emit_signal("rotation_changed", new_rotation);
            queue_redraw();
            break;
        }
        default:
            break;
    }
}
