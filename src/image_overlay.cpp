#include "image_overlay.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

void ImageOverlay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture_path", "path"), &ImageOverlay::set_texture_path);
    ClassDB::bind_method(D_METHOD("get_texture_path"), &ImageOverlay::get_texture_path);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::STRING, "texture_path"), "set_texture_path", "get_texture_path");

    ClassDB::bind_method(D_METHOD("set_position", "pos"), &ImageOverlay::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &ImageOverlay::get_position);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::VECTOR2, "position"), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("set_scale", "scale"), &ImageOverlay::set_scale);
    ClassDB::bind_method(D_METHOD("get_scale"), &ImageOverlay::get_scale);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::VECTOR2, "scale"), "set_scale", "get_scale");

    ClassDB::bind_method(D_METHOD("set_rotation", "rotation"), &ImageOverlay::set_rotation);
    ClassDB::bind_method(D_METHOD("get_rotation"), &ImageOverlay::get_rotation);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::FLOAT, "rotation"), "set_rotation", "get_rotation");

    ClassDB::bind_method(D_METHOD("set_opacity", "opacity"), &ImageOverlay::set_opacity);
    ClassDB::bind_method(D_METHOD("get_opacity"), &ImageOverlay::get_opacity);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::FLOAT, "opacity", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_opacity", "get_opacity");

    ClassDB::bind_method(D_METHOD("set_anchor_point", "anchor"), &ImageOverlay::set_anchor_point);
    ClassDB::bind_method(D_METHOD("get_anchor_point"), &ImageOverlay::get_anchor_point);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::VECTOR2, "anchor_point"), "set_anchor_point", "get_anchor_point");

    ClassDB::bind_method(D_METHOD("set_start_time", "time"), &ImageOverlay::set_start_time);
    ClassDB::bind_method(D_METHOD("get_start_time"), &ImageOverlay::get_start_time);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::FLOAT, "start_time"), "set_start_time", "get_start_time");

    ClassDB::bind_method(D_METHOD("set_end_time", "time"), &ImageOverlay::set_end_time);
    ClassDB::bind_method(D_METHOD("get_end_time"), &ImageOverlay::get_end_time);
    ClassDB::add_property("ImageOverlay", PropertyInfo(Variant::FLOAT, "end_time"), "set_end_time", "get_end_time");

    ClassDB::bind_method(D_METHOD("is_visible_at", "time"), &ImageOverlay::is_visible_at);
    ClassDB::bind_method(D_METHOD("render_to_image"), &ImageOverlay::render_to_image);
    ClassDB::bind_method(D_METHOD("get_image_size"), &ImageOverlay::get_image_size);
}

ImageOverlay::ImageOverlay() {}
ImageOverlay::~ImageOverlay() {
    _free_resources();
}

void ImageOverlay::set_texture_path(const String &p_path) {
    texture_path = p_path;
    image_loaded = false;
    cached_image.unref();
    cached_texture.unref();
    resources_dirty = true;
}

String ImageOverlay::get_texture_path() const {
    return texture_path;
}

void ImageOverlay::set_position(const Vector2 &p_pos) { position = p_pos; }
Vector2 ImageOverlay::get_position() const { return position; }

void ImageOverlay::set_scale(const Vector2 &p_scale) { scale = p_scale; }
Vector2 ImageOverlay::get_scale() const { return scale; }

void ImageOverlay::set_rotation(float p_rot) { rotation = p_rot; }
float ImageOverlay::get_rotation() const { return rotation; }

void ImageOverlay::set_opacity(float p_opacity) { opacity = CLAMP(p_opacity, 0.0f, 1.0f); }
float ImageOverlay::get_opacity() const { return opacity; }

void ImageOverlay::set_anchor_point(const Vector2 &p_anchor) { anchor_point = p_anchor; }
Vector2 ImageOverlay::get_anchor_point() const { return anchor_point; }

void ImageOverlay::set_start_time(double p_time) { start_time = p_time; }
double ImageOverlay::get_start_time() const { return start_time; }

void ImageOverlay::set_end_time(double p_time) { end_time = p_time; }
double ImageOverlay::get_end_time() const { return end_time; }

bool ImageOverlay::is_visible_at(double p_time) const {
    return p_time >= start_time && p_time < end_time;
}

void ImageOverlay::_ensure_resources(RenderingServer *p_rs, int p_width, int p_height) {
    if (!image_loaded && !texture_path.is_empty()) {
        String resolved = texture_path;
        if (resolved.begins_with("user://") || resolved.begins_with("res://")) {
            ProjectSettings *ps = ProjectSettings::get_singleton();
            if (ps) resolved = ps->globalize_path(resolved);
        }
        cached_image = Image::load_from_file(resolved);
        if (cached_image.is_valid()) {
            cached_texture.instantiate();
            cached_texture->set_image(cached_image);
            image_loaded = true;
        } else {
            UtilityFunctions::push_error("[ImageOverlay] Failed to load: ", texture_path);
        }
    }

    if (overlay_viewport.is_valid() && cached_w == p_width && cached_h == p_height && !resources_dirty) {
        return;
    }

    _free_resources();

    overlay_viewport = p_rs->viewport_create();
    p_rs->viewport_set_size(overlay_viewport, p_width, p_height);
    p_rs->viewport_set_transparent_background(overlay_viewport, true);
    p_rs->viewport_set_active(overlay_viewport, true);
    p_rs->viewport_set_update_mode(overlay_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);

    overlay_canvas = p_rs->canvas_create();
    overlay_item = p_rs->canvas_item_create();
    p_rs->canvas_item_set_parent(overlay_item, overlay_canvas);
    p_rs->viewport_attach_canvas(overlay_viewport, overlay_canvas);

    cached_w = p_width;
    cached_h = p_height;
    resources_dirty = false;
}

void ImageOverlay::_free_resources() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return;
    if (overlay_item.is_valid()) { p_rs->free_rid(overlay_item); overlay_item = RID(); }
    if (overlay_canvas.is_valid()) { p_rs->free_rid(overlay_canvas); overlay_canvas = RID(); }
    if (overlay_viewport.is_valid()) { p_rs->free_rid(overlay_viewport); overlay_viewport = RID(); }
    cached_w = 0;
    cached_h = 0;
}

RID ImageOverlay::render_to_rid(RenderingServer *p_rs, int p_canvas_w, int p_canvas_h, double p_time) {
    if (!is_visible_at(p_time)) return RID();
    _ensure_resources(p_rs, p_canvas_w, p_canvas_h);
    if (!overlay_viewport.is_valid()) return RID();

    p_rs->canvas_item_clear(overlay_item);

    if (cached_texture.is_valid()) {
        RID tex_rid = cached_texture->get_rid();
        int img_w = cached_image->get_width();
        int img_h = cached_image->get_height();

        Transform2D t;
        t[0] = Vector2(Math::cos(rotation), Math::sin(rotation)) * scale.x;
        t[1] = Vector2(-Math::sin(rotation), Math::cos(rotation)) * scale.y;
        Vector2 anchor_offset = Vector2(anchor_point.x * (float)img_w, anchor_point.y * (float)img_h);
        t[2] = position - (t[0] * anchor_offset.x + t[1] * anchor_offset.y);

        p_rs->canvas_item_set_transform(overlay_item, t);
        Color modulate = Color(1.0f, 1.0f, 1.0f, opacity);
        p_rs->canvas_item_set_self_modulate(overlay_item, modulate);
        p_rs->canvas_item_add_texture_rect(overlay_item, Rect2(0, 0, img_w, img_h), tex_rid);
    }

    p_rs->viewport_set_update_mode(overlay_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    return p_rs->viewport_get_texture(overlay_viewport);
}

Ref<Image> ImageOverlay::render_to_image() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return Ref<Image>();
    _ensure_resources(p_rs, 1080, 1920);
    if (!overlay_viewport.is_valid()) return Ref<Image>();

    p_rs->viewport_set_update_mode(overlay_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    RID tex = p_rs->viewport_get_texture(overlay_viewport);
    return p_rs->texture_2d_get(tex);
}
