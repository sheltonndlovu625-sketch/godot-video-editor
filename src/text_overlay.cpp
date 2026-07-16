#include "text_overlay.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

void TextOverlay::_bind_methods() {
    // Content
    ClassDB::bind_method(D_METHOD("set_text", "text"), &TextOverlay::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &TextOverlay::get_text);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");

    ClassDB::bind_method(D_METHOD("set_font", "font"), &TextOverlay::set_font);
    ClassDB::bind_method(D_METHOD("get_font"), &TextOverlay::get_font);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::OBJECT, "font"), "set_font", "get_font");

    ClassDB::bind_method(D_METHOD("set_font_size", "size"), &TextOverlay::set_font_size);
    ClassDB::bind_method(D_METHOD("get_font_size"), &TextOverlay::get_font_size);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::INT, "font_size"), "set_font_size", "get_font_size");

    // Style
    ClassDB::bind_method(D_METHOD("set_color", "color"), &TextOverlay::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &TextOverlay::get_color);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");

    ClassDB::bind_method(D_METHOD("set_outline_size", "size"), &TextOverlay::set_outline_size);
    ClassDB::bind_method(D_METHOD("get_outline_size"), &TextOverlay::get_outline_size);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::INT, "outline_size"), "set_outline_size", "get_outline_size");

    ClassDB::bind_method(D_METHOD("set_outline_color", "color"), &TextOverlay::set_outline_color);
    ClassDB::bind_method(D_METHOD("get_outline_color"), &TextOverlay::get_outline_color);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::COLOR, "outline_color"), "set_outline_color", "get_outline_color");

    ClassDB::bind_method(D_METHOD("set_shadow_offset", "offset"), &TextOverlay::set_shadow_offset);
    ClassDB::bind_method(D_METHOD("get_shadow_offset"), &TextOverlay::get_shadow_offset);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::VECTOR2, "shadow_offset"), "set_shadow_offset", "get_shadow_offset");

    ClassDB::bind_method(D_METHOD("set_shadow_color", "color"), &TextOverlay::set_shadow_color);
    ClassDB::bind_method(D_METHOD("get_shadow_color"), &TextOverlay::get_shadow_color);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::COLOR, "shadow_color"), "set_shadow_color", "get_shadow_color");

    ClassDB::bind_method(D_METHOD("set_background_color", "color"), &TextOverlay::set_background_color);
    ClassDB::bind_method(D_METHOD("get_background_color"), &TextOverlay::get_background_color);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");

    ClassDB::bind_method(D_METHOD("set_padding", "padding"), &TextOverlay::set_padding);
    ClassDB::bind_method(D_METHOD("get_padding"), &TextOverlay::get_padding);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::VECTOR2, "padding"), "set_padding", "get_padding");

    // Transform
    ClassDB::bind_method(D_METHOD("set_position", "pos"), &TextOverlay::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &TextOverlay::get_position);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::VECTOR2, "position"), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("set_anchor_point", "anchor"), &TextOverlay::set_anchor_point);
    ClassDB::bind_method(D_METHOD("get_anchor_point"), &TextOverlay::get_anchor_point);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::VECTOR2, "anchor_point"), "set_anchor_point", "get_anchor_point");

    ClassDB::bind_method(D_METHOD("set_opacity", "opacity"), &TextOverlay::set_opacity);
    ClassDB::bind_method(D_METHOD("get_opacity"), &TextOverlay::get_opacity);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::FLOAT, "opacity", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_opacity", "get_opacity");

    // Timing
    ClassDB::bind_method(D_METHOD("set_start_time", "time"), &TextOverlay::set_start_time);
    ClassDB::bind_method(D_METHOD("get_start_time"), &TextOverlay::get_start_time);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::FLOAT, "start_time"), "set_start_time", "get_start_time");

    ClassDB::bind_method(D_METHOD("set_end_time", "time"), &TextOverlay::set_end_time);
    ClassDB::bind_method(D_METHOD("get_end_time"), &TextOverlay::get_end_time);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::FLOAT, "end_time"), "set_end_time", "get_end_time");

    ClassDB::bind_method(D_METHOD("is_visible_at", "time"), &TextOverlay::is_visible_at);

    // Animation
    ClassDB::bind_method(D_METHOD("set_animation", "anim"), &TextOverlay::set_animation);
    ClassDB::bind_method(D_METHOD("get_animation"), &TextOverlay::get_animation);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::INT, "animation"), "set_animation", "get_animation");

    ClassDB::bind_method(D_METHOD("set_animation_duration", "duration"), &TextOverlay::set_animation_duration);
    ClassDB::bind_method(D_METHOD("get_animation_duration"), &TextOverlay::get_animation_duration);
    ClassDB::add_property("TextOverlay", PropertyInfo(Variant::FLOAT, "animation_duration"), "set_animation_duration", "get_animation_duration");

    ClassDB::bind_method(D_METHOD("render_to_image"), &TextOverlay::render_to_image);
    ClassDB::bind_method(D_METHOD("mark_dirty"), &TextOverlay::mark_dirty);

    BIND_ENUM_CONSTANT(ANIM_NONE);
    BIND_ENUM_CONSTANT(ANIM_FADE_IN);
    BIND_ENUM_CONSTANT(ANIM_FADE_OUT);
    BIND_ENUM_CONSTANT(ANIM_SLIDE_IN_LEFT);
    BIND_ENUM_CONSTANT(ANIM_SLIDE_IN_RIGHT);
    BIND_ENUM_CONSTANT(ANIM_SLIDE_IN_TOP);
    BIND_ENUM_CONSTANT(ANIM_SLIDE_IN_BOTTOM);
    BIND_ENUM_CONSTANT(ANIM_BOUNCE);
    BIND_ENUM_CONSTANT(ANIM_TYPEWRITER);
}

TextOverlay::TextOverlay() {}
TextOverlay::~TextOverlay() {
    _free_text_resources();
}

// ---- Setters / Getters ----

void TextOverlay::set_text(const String &p_text) {
    text = p_text;
    text_dirty = true;
}
String TextOverlay::get_text() const { return text; }

void TextOverlay::set_font(const Ref<Font> &p_font) {
    font = p_font;
    text_dirty = true;
}
Ref<Font> TextOverlay::get_font() const { return font; }

void TextOverlay::set_font_size(int p_size) {
    font_size = p_size;
    text_dirty = true;
}
int TextOverlay::get_font_size() const { return font_size; }

void TextOverlay::set_color(const Color &p_color) { color = p_color; }
Color TextOverlay::get_color() const { return color; }

void TextOverlay::set_outline_size(int p_size) { outline_size = p_size; }
int TextOverlay::get_outline_size() const { return outline_size; }

void TextOverlay::set_outline_color(const Color &p_color) { outline_color = p_color; }
Color TextOverlay::get_outline_color() const { return outline_color; }

void TextOverlay::set_shadow_offset(const Vector2 &p_offset) { shadow_offset = p_offset; }
Vector2 TextOverlay::get_shadow_offset() const { return shadow_offset; }

void TextOverlay::set_shadow_color(const Color &p_color) { shadow_color = p_color; }
Color TextOverlay::get_shadow_color() const { return shadow_color; }

void TextOverlay::set_background_color(const Color &p_color) { background_color = p_color; }
Color TextOverlay::get_background_color() const { return background_color; }

void TextOverlay::set_padding(const Vector2 &p_padding) { padding = p_padding; }
Vector2 TextOverlay::get_padding() const { return padding; }

void TextOverlay::set_position(const Vector2 &p_pos) { position = p_pos; }
Vector2 TextOverlay::get_position() const { return position; }

void TextOverlay::set_anchor_point(const Vector2 &p_anchor) { anchor_point = p_anchor; }
Vector2 TextOverlay::get_anchor_point() const { return anchor_point; }

void TextOverlay::set_opacity(float p_opacity) { opacity = CLAMP(p_opacity, 0.0f, 1.0f); }
float TextOverlay::get_opacity() const { return opacity; }

void TextOverlay::set_start_time(double p_time) { start_time = p_time; }
double TextOverlay::get_start_time() const { return start_time; }

void TextOverlay::set_end_time(double p_time) { end_time = p_time; }
double TextOverlay::get_end_time() const { return end_time; }

bool TextOverlay::is_visible_at(double p_time) const {
    return p_time >= start_time && p_time < end_time;
}

void TextOverlay::set_animation(int p_anim) { animation = (AnimationType)p_anim; }
int TextOverlay::get_animation() const { return (int)animation; }

void TextOverlay::set_animation_duration(double p_duration) { animation_duration = p_duration; }
double TextOverlay::get_animation_duration() const { return animation_duration; }

void TextOverlay::mark_dirty() { text_dirty = true; }

// ---- Internal helpers ----

Vector2 TextOverlay::_compute_text_size() const {
    if (font.is_null() || text.is_empty()) {
        return Vector2(100.0f, (float)font_size);
    }
    // Use Font::get_string_size for accurate sizing
    // In godot-cpp, get_string_size signature may vary; use basic call
    Vector2 size = font->get_string_size(text, 0, -1.0f, font_size);
    return size;
}

void TextOverlay::_free_text_resources() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return;
    if (text_item.is_valid()) { p_rs->free_rid(text_item); text_item = RID(); }
    if (text_canvas.is_valid()) { p_rs->free_rid(text_canvas); text_canvas = RID(); }
    if (text_viewport.is_valid()) { p_rs->free_rid(text_viewport); text_viewport = RID(); }
    cached_text_w = 0;
    cached_text_h = 0;
}

void TextOverlay::_ensure_text_resources(RenderingServer *p_rs) {
    Vector2 text_size = _compute_text_size();
    int vw = int(text_size.x + padding.x * 2.0f);
    int vh = int(text_size.y + padding.y * 2.0f);
    if (vw < 4) vw = 4;
    if (vh < 4) vh = 4;

    if (text_viewport.is_valid() && cached_text_w == vw && cached_text_h == vh && !text_dirty) {
        return;
    }

    _free_text_resources();

    text_viewport = p_rs->viewport_create();
    p_rs->viewport_set_size(text_viewport, vw, vh);
    p_rs->viewport_set_transparent_background(text_viewport, true);
    p_rs->viewport_set_active(text_viewport, true);
    p_rs->viewport_set_update_mode(text_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);

    text_canvas = p_rs->canvas_create();
    text_item = p_rs->canvas_item_create();
    p_rs->canvas_item_set_parent(text_item, text_canvas);
    p_rs->viewport_attach_canvas(text_viewport, text_canvas);

    // Draw background
    if (background_color.a > 0.001f) {
        p_rs->canvas_item_add_rect(text_item, Rect2(0, 0, vw, vh), background_color);
    }

    // Draw shadow (offset copy of text)
    if (shadow_color.a > 0.001f && font.is_valid() && !text.is_empty()) {
        Vector2 shadow_pos = padding + shadow_offset;
        font->draw_string(text_item, shadow_pos, text, 0, -1.0f, font_size, shadow_color, 0, Color(1, 1, 1, 1));
    }

    // Draw main text with outline
    if (font.is_valid() && !text.is_empty()) {
        Vector2 text_pos = padding;
        font->draw_string(text_item, text_pos, text, 0, -1.0f, font_size, color, outline_size, outline_color);
    }

    cached_text_w = vw;
    cached_text_h = vh;
    text_dirty = false;
}

void TextOverlay::_apply_animation(double p_local_time, float &r_opacity, Vector2 &r_position_offset) {
    r_opacity = opacity;
    r_position_offset = Vector2(0, 0);

    if (animation == ANIM_NONE) return;

    double t = p_local_time / animation_duration;
    if (t > 1.0) t = 1.0;
    if (t < 0.0) t = 0.0;

    switch (animation) {
        case ANIM_FADE_IN: {
            r_opacity = opacity * (float)t;
            break;
        }
        case ANIM_FADE_OUT: {
            r_opacity = opacity * (float)(1.0 - t);
            break;
        }
        case ANIM_SLIDE_IN_LEFT: {
            float ease = 1.0f - Math::pow(1.0f - (float)t, 3.0f);
            r_position_offset.x = -(float)cached_text_w * (1.0f - ease);
            break;
        }
        case ANIM_SLIDE_IN_RIGHT: {
            float ease = 1.0f - Math::pow(1.0f - (float)t, 3.0f);
            r_position_offset.x = (float)cached_text_w * (1.0f - ease);
            break;
        }
        case ANIM_SLIDE_IN_TOP: {
            float ease = 1.0f - Math::pow(1.0f - (float)t, 3.0f);
            r_position_offset.y = -(float)cached_text_h * (1.0f - ease);
            break;
        }
        case ANIM_SLIDE_IN_BOTTOM: {
            float ease = 1.0f - Math::pow(1.0f - (float)t, 3.0f);
            r_position_offset.y = (float)cached_text_h * (1.0f - ease);
            break;
        }
        case ANIM_BOUNCE: {
            float bounce = Math::sin((float)t * Math_PI * 4.0f) * Math::exp(-(float)t * 4.0f);
            r_position_offset.y = bounce * 20.0f;
            break;
        }
        case ANIM_TYPEWRITER: {
            r_opacity = opacity * (float)t;
            break;
        }
        default:
            break;
    }
}

// ---- Public render methods ----

RID TextOverlay::render_to_rid(RenderingServer *p_rs, int p_canvas_w, int p_canvas_h, double p_time) {
    if (!is_visible_at(p_time)) return RID();

    _ensure_text_resources(p_rs);
    if (!text_viewport.is_valid()) return RID();

    double local_time = p_time - start_time;
    float anim_opacity;
    Vector2 anim_offset;
    _apply_animation(local_time, anim_opacity, anim_offset);

    Vector2 final_pos = position + anim_offset;
    Vector2 anchor_offset = Vector2(anchor_point.x * (float)cached_text_w, anchor_point.y * (float)cached_text_h);
    final_pos -= anchor_offset;

    Color modulate = Color(1.0f, 1.0f, 1.0f, anim_opacity);
    p_rs->canvas_item_set_self_modulate(text_item, modulate);

    p_rs->viewport_set_update_mode(text_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    return p_rs->viewport_get_texture(text_viewport);
}

Ref<Image> TextOverlay::render_to_image() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return Ref<Image>();

    _ensure_text_resources(p_rs);
    if (!text_viewport.is_valid()) return Ref<Image>();

    p_rs->viewport_set_update_mode(text_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);

    RID tex = p_rs->viewport_get_texture(text_viewport);
    return p_rs->texture_2d_get(tex);
}
