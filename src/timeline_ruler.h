#ifndef TIMELINE_RULER_H
#define TIMELINE_RULER_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class TimelineRuler : public Control {
    GDCLASS(TimelineRuler, Control)

private:
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    double duration = 60.0;
    float header_width = 50.0f;

    // Inspector customisation
    Ref<Font> font;
    int font_size = 9;
    Color font_color = Color(0.5f, 0.5f, 0.5f);
    Color bg_color = Color(0.08f, 0.08f, 0.10f);
    Color tick_color = Color(0.5f, 0.5f, 0.5f);

protected:
    static void _bind_methods();

public:
    void _draw() override;

    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_duration(double p_duration);
    double get_duration() const;
    void set_header_width(float p_width);
    float get_header_width() const;

    // Customisation
    void set_font(const Ref<Font> &p_font);
    Ref<Font> get_font() const;
    void set_font_size(int p_size);
    int get_font_size() const;
    void set_font_color(const Color &p_color);
    Color get_font_color() const;
    void set_bg_color(const Color &p_color);
    Color get_bg_color() const;
    void set_tick_color(const Color &p_color);
    Color get_tick_color() const;

    TimelineRuler();
};

}
#endif
