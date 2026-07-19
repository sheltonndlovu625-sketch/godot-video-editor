#include "timeline_ruler.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimelineRuler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineRuler::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineRuler::get_pixels_per_second);
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineRuler::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineRuler::get_zoom);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &TimelineRuler::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &TimelineRuler::get_duration);
    ClassDB::bind_method(D_METHOD("set_header_width", "width"), &TimelineRuler::set_header_width);
    ClassDB::bind_method(D_METHOD("get_header_width"), &TimelineRuler::get_header_width);
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

void TimelineRuler::_draw() {
    float h = get_size().y;
    draw_rect(Rect2(0, 0, get_size().x, h), Color(0.08f, 0.08f, 0.10f));

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

        draw_line(Vector2(x, h - tick_h), Vector2(x, h), Color(0.5f, 0.5f, 0.5f), 1.0f);

        if (major) {
            int mins = int(t) / 60;
            int secs = int(t) % 60;
            String label = String::num_int64(mins).pad_zeros(2) + ":" + String::num_int64(secs).pad_zeros(2);
            draw_string(get_theme_default_font(), Vector2(x + 3, h - 6), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 9, Color(0.5f, 0.5f, 0.5f));
        }

        t += step;
    }
}
