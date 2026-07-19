#include "timeline_ruler.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimelineRuler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineRuler::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineRuler::get_pixels_per_second);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::FLOAT, "pixels_per_second"), "set_pixels_per_second", "get_pixels_per_second");

    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineRuler::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineRuler::get_zoom);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &TimelineRuler::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &TimelineRuler::get_duration);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");

    ClassDB::bind_method(D_METHOD("set_header_width", "width"), &TimelineRuler::set_header_width);
    ClassDB::bind_method(D_METHOD("get_header_width"), &TimelineRuler::get_header_width);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::FLOAT, "header_width"), "set_header_width", "get_header_width");

    // Customisation bindings
    ClassDB::bind_method(D_METHOD("set_font", "font"), &TimelineRuler::set_font);
    ClassDB::bind_method(D_METHOD("get_font"), &TimelineRuler::get_font);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::OBJECT, "font", PROPERTY_HINT_RESOURCE_TYPE, "Font"), "set_font", "get_font");

    ClassDB::bind_method(D_METHOD("set_font_size", "size"), &TimelineRuler::set_font_size);
    ClassDB::bind_method(D_METHOD("get_font_size"), &TimelineRuler::get_font_size);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::INT, "font_size"), "set_font_size", "get_font_size");

    ClassDB::bind_method(D_METHOD("set_font_color", "color"), &TimelineRuler::set_font_color);
    ClassDB::bind_method(D_METHOD("get_font_color"), &TimelineRuler::get_font_color);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::COLOR, "font_color"), "set_font_color", "get_font_color");

    ClassDB::bind_method(D_METHOD("set_bg_color", "color"), &TimelineRuler::set_bg_color);
    ClassDB::bind_method(D_METHOD("get_bg_color"), &TimelineRuler::get_bg_color);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::COLOR, "bg_color"), "set_bg_color", "get_bg_color");

    ClassDB::bind_method(D_METHOD("set_tick_color", "color"), &TimelineRuler::set_tick_color);
    ClassDB::bind_method(D_METHOD("get_tick_color"), &TimelineRuler::get_tick_color);
    ClassDB::add_property("TimelineRuler", PropertyInfo(Variant::COLOR, "tick_color"), "set_tick_color", "get_tick_color");
}

TimelineRuler::TimelineRuler() {}

void TimelineRuler::set_pixels_per_second(float p_pps) {
    pixels_per_second = p_pps;
    queue_redraw();
}

float TimelineRuler::get_pixels_per_second() const {
    return pixels_per_second;
}

void TimelineRuler::set_zoom(float p_zoom) {
    zoom = p_zoom;
    queue_redraw();
}

float TimelineRuler::get_zoom() const {
    return zoom;
}

void TimelineRuler::set_duration(double p_duration) {
    duration = p_duration;
    queue_redraw();
}

double TimelineRuler::get_duration() const {
    return duration;
}

void TimelineRuler::set_header_width(float p_width) {
    header_width = p_width;
    queue_redraw();
}

float TimelineRuler::get_header_width() const {
    return header_width;
}

// ---- Customisation ----

void TimelineRuler::set_font(const Ref<Font> &p_font) {
    font = p_font;
    queue_redraw();
}

Ref<Font> TimelineRuler::get_font() const {
    return font;
}

void TimelineRuler::set_font_size(int p_size) {
    font_size = Math::max(1, p_size);
    queue_redraw();
}

int TimelineRuler::get_font_size() const {
    return font_size;
}

void TimelineRuler::set_font_color(const Color &p_color) {
    font_color = p_color;
    queue_redraw();
}

Color TimelineRuler::get_font_color() const {
    return font_color;
}

void TimelineRuler::set_bg_color(const Color &p_color) {
    bg_color = p_color;
    queue_redraw();
}

Color TimelineRuler::get_bg_color() const {
    return bg_color;
}

void TimelineRuler::set_tick_color(const Color &p_color) {
    tick_color = p_color;
    queue_redraw();
}

Color TimelineRuler::get_tick_color() const {
    return tick_color;
}

// ---- Drawing ----

void TimelineRuler::_draw() {
    float h = get_size().y;
    draw_rect(Rect2(0, 0, get_size().x, h), bg_color);

    Ref<Font> draw_font = font.is_valid() ? font : get_theme_default_font();
    int draw_size = font_size > 0 ? font_size : 9;

    float pps = pixels_per_second * zoom;
    float step = 1.0f;
    if (pps < 20.0f) step = 5.0f;
    else if (pps > 120.0f) step = 0.5f;

    double t = 0.0;
    while (t <= duration) {
        float x = header_width + t * pps;
        if (x > get_size().x) break;

        bool major = Math::fmod(t, (double)Math::max(step * 2.0f, 1.0f)) < 0.01;
        float tick_h = major ? h * 0.6f : h * 0.25f;

        draw_line(Vector2(x, h - tick_h), Vector2(x, h), tick_color, 1.0f);

        if (major) {
            int mins = int(t) / 60;
            int secs = int(t) % 60;
            String label = String::num_int64(mins).pad_zeros(2) + ":" + String::num_int64(secs).pad_zeros(2);
            draw_string(draw_font, Vector2(x + 3, h - 6), label, HORIZONTAL_ALIGNMENT_LEFT, -1, draw_size, font_color);
        }

        t += step;
    }
}
